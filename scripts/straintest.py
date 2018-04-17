import os
import errno
import numpy as np
import argparse
from .sph import bindings as bnd
import time
import ctypes
import math

from functools import reduce

# Get column indices where x[:,i] is in the columns of y
def column_intersection_indices(x, y):
    return np.where(reduce(np.logical_or,
                           tuple(np.all(x==y[i,:], axis=1)
                                 for i in range(y.shape[0])) ))[0]

class StrainTest(object):
    def __init__(self, **params):
        required = ["strain_range", "axis", "strain_rate", "stabilisation_time",
                    "initial_dimensions", "particle_dimensions", "total_mass", "strain_solid",
                    "output_particles", "output_fields", "history_particles", "history_fields"]

        missing = [key for key in required if not key in params]

        if len(missing) > 0:
            raise Exception("Missing required fields: {}".format(missing))

        self.params = argparse.Namespace(**params)

        if hasattr(self.params, "output_folder"):
            try:
                os.makedirs(self.params.output_folder)
            except OSError as e:
                if e.errno == errno.EEXIST and os.path.isdir(self.params.output_folder):
                    pass
                else:
                    raise

        ps = self.params.strain_solid

        self.init_positions()

        self.init_particle_system()

        self.post_init()

        self.current_strain = 0;
        self.plane_frame_step = self.params.strain_rate * ps.get_fields("timestep")[0] * self.params.initial_dimensions[self.params.axis] / 2
        self.stabilisation_steps = int(self.params.stabilisation_time / ps.get_fields("timestep")[0])

        self.init_output_and_history_indices()
        self.init_run_path()
        self.init_output_arrays()
        self.init_history_arrays()

        ps.sync_host_to_device()

    def init_positions(self):
        p = self.params

        initdims = p.initial_dimensions
        pdims = p.particle_dimensions

        # Generate particle positions
        x, y, z = np.meshgrid(np.linspace(-initdims[0]/2, initdims[0]/2, num=pdims[0]),
                              np.linspace(-initdims[1]/2, initdims[1]/2, num=pdims[1]),
                              np.linspace(-initdims[2]/2, initdims[2]/2, num=pdims[2]),
                              indexing='ij')

        self.positions = np.vstack((x.flatten(), y.flatten(), z.flatten())).T

    def init_particle_system(self):
        if hasattr(self.params, "smoothingradius"):
            self.params.strain_solid.init(position=self.positions, n=self.positions.shape[0], mass=self.params.total_mass/np.product(self.params.particle_dimensions-1), smoothingradius=self.params.smoothingradius)
        else:
            self.params.strain_solid.init(position=self.positions, n=self.positions.shape[0], mass=self.params.total_mass/np.product(self.params.particle_dimensions-1))

    def init_output_and_history_indices(self):
        pdims = self.params.particle_dimensions

        x, y, z = np.meshgrid(np.arange(pdims[0]), np.arange(pdims[1]), np.arange(pdims[2]), indexing='ij')

        indices = np.vstack((x.flatten(), y.flatten(), z.flatten())).T

        self.output_particle_indices = column_intersection_indices(indices, self.params.output_particles)

        if self.params.history_particles == "all":
            self.history_particle_indices = np.arange(self.positions.shape[0])
        else:
            self.history_particle_indices = column_intersection_indices(indices, self.history_particles)

    def post_init(self):
        self.init_plane_constraints()

    def init_plane_constraints(self):
        # Constrain the planes normal to axis
        self.plane_constraints, self.plane_constraints_particles =\
            self.params.strain_solid.get_fields("plane_constraints", "plane_constraints_particles")

        initdims = self.params.initial_dimensions

        axis = self.params.axis

        # [constraint plane, 0: point on plane 1: normal, vector axis]
        self.plane_constraints[0, :, axis] = [-initdims[axis]/2, 1]
        self.plane_constraints[1, :, axis] = [initdims[axis]/2, 1]

        self.plane_constraints_particles[np.where(self.positions[:, axis] == -initdims[axis]/2)] = 1 << 0
        self.plane_constraints_particles[np.where(self.positions[:, axis] == initdims[axis]/2)] = 1 << 1

    def get_init_plane_position(self):
        return self.params.initial_dimensions[self.params.axis] / 2

    def init_run_path(self):
        self.run_path = []

        tension = []
        compression = []

        p = self.params

        tension_order = np.where(p.strain_range >= 0)[0]
        compression_order = np.flipud(np.where(p.strain_range < 0)[0])

        self.strain_order = np.concatenate((tension_order, compression_order))

        init_halfwidth = self.get_init_plane_position()

        tension_plane_positions = (p.strain_range[tension_order] + 1) * init_halfwidth
        compression_plane_positions = (p.strain_range[compression_order] + 1) * init_halfwidth

        tension_plane_positions = np.concatenate((np.array([init_halfwidth]), tension_plane_positions))
        compression_plane_positions = np.concatenate((np.array([init_halfwidth]), compression_plane_positions))

        for i in range(1, len(tension_plane_positions)):
            move_stage = np.arange(tension_plane_positions[i-1], tension_plane_positions[i], self.plane_frame_step)
            stay_stage = np.ones(self.stabilisation_steps, dtype=np.int64) * tension_plane_positions[i]
            tension.append(np.concatenate((move_stage, stay_stage)))

        for i in range(1, len(compression_plane_positions)):
            move_stage = -np.arange(-compression_plane_positions[i-1], -compression_plane_positions[i], self.plane_frame_step)
            stay_stage = np.ones(self.stabilisation_steps, dtype=np.int64) * compression_plane_positions[i]
            compression.append(np.concatenate((move_stage, stay_stage)))

        self.run_path.append(tension)
        self.run_path.append(compression)

        self.run_step_count = sum(map(len, tension)) + sum(map(len, compression))

    def init_output_arrays(self):
        p = self.params

        prototype_output_arrays = p.strain_solid.get_fields(*p.output_fields)

        if hasattr(p, "output_folder"):
            self.output = { p.output_fields[i]: np.lib.format.open_memmap(p.output_folder+"/"+p.output_fields[i]+"_output.npy",
                                                            dtype=prototype_output_arrays[i].dtype,
                                                            mode='w+',
                                                            shape=(p.strain_range.size, self.output_particle_indices.size) + prototype_output_arrays[i].shape[1:])
                                 for i in range(len(p.output_fields)) }
        else:
            self.output = { p.output_fields[i]: np.zeros((p.strain_range.size, self.output_particle_indices.size) + prototype_output_arrays[i].shape[1:])
                            for i in range(len(p.output_fields)) }

    def init_history_arrays(self):
        p = self.params

        prototype_history_arrays = p.strain_solid.get_fields(*p.history_fields)

        if hasattr(p, "output_folder"):
            self.history = { p.history_fields[i]: np.lib.format.open_memmap(p.output_folder+"/"+p.history_fields[i]+"_history.npy",
                                                            dtype=prototype_history_arrays[i].dtype,
                                                            mode='w+',
                                                            shape=(self.run_step_count, self.history_particle_indices.size) + prototype_history_arrays[i].shape[1:])
                                 for i in range(len(p.history_fields)) }
        else:
            self.history = { p.history_fields[i]: np.zeros((self.run_step_count, self.history_particle_indices.size) + prototype_history_arrays[i].shape[1:])
                             for i in range(len(p.history_fields)) }

        self.history_counter = 0

    def record_output(self, sample_index):
        p = self.params

        for key in self.output:
            np.copyto(self.output[key][sample_index], p.strain_solid.get_fields(key)[0][self.output_particle_indices])

    def record_history(self):
        if (self.history_counter >= self.run_step_count):
            raise Exception("History is full")

        p = self.params

        for key in self.history:
            np.copyto(self.history[key][self.history_counter], p.strain_solid.get_fields(key)[0][self.history_particle_indices])

        self.history_counter += 1

    def set_plane_constraints(self, plane_position):
        self.plane_constraints[0, 0, self.params.axis] = -plane_position
        self.plane_constraints[1, 0, self.params.axis] = plane_position

    def detect_failure(self, **data):
        return np.any(np.isnan(data['position']))

    def run(self, **kwargs):
        p = self.params

        strip_index = 0

        union_history_position = list(set(p.output_fields + p.history_fields))

        p.strain_solid.setup()

        for section in self.run_path:
            for strip in section:
                for plane_position in strip:
                    self.set_plane_constraints(plane_position)
                    p.strain_solid.sync_fields_host_to_device("plane_constraints")

                    p.strain_solid.update()

                    p.strain_solid.call_for_all_particles("apply_plane_constraints")

                    p.strain_solid.sync_fields_device_to_host(*union_history_position)

                    self.record_history()

                    if self.detect_failure(position=self.history['position'][self.history_counter-1]):
                        p.strain_solid.sync_fields_device_to_host()

                        print("Something exploded or got set to nan: stopping run")
                        print("The fields from the last frame are available in params['strain_solid']")

                        return "failure"
        
                print("Strain: {}".format(self.params.strain_range[self.strain_order[strip_index]]))
                self.record_output(self.strain_order[strip_index])
                strip_index += 1

            p.strain_solid.reset()
            self.post_init()
            p.strain_solid.sync_host_to_device()
            p.strain_solid.setup()

        return "success"

class StrainTestSym(StrainTest):
    def __init__(self, **params):
        super(StrainTestSym, self).__init__(**params)

    def init_particle_system(self):
        if hasattr(self.params, "smoothingradius"):
            self.params.strain_solid.init(position=self.positions, n=self.positions.shape[0], mass=self.params.total_mass/np.product(2*(self.params.particle_dimensions-1)), smoothingradius=self.params.smoothingradius)
        else:
            self.params.strain_solid.init(position=self.positions, n=self.positions.shape[0], mass=self.params.total_mass/np.product(2*(self.params.particle_dimensions-1)))

    def init_positions(self):
        p = self.params

        initdims = p.initial_dimensions
        pdims = p.particle_dimensions

        # Generate particle positions
        x, y, z = np.meshgrid(np.linspace(0, initdims[0]/2, num=pdims[0]),
                            np.linspace(0, initdims[1]/2, num=pdims[1]),
                            np.linspace(0, initdims[2]/2, num=pdims[2]),
                            indexing='ij')

        self.positions = np.vstack((x.flatten(), y.flatten(), z.flatten())).T

    def init_plane_constraints(self):
        # Constrain the planes normal to axis
        self.plane_constraints, self.plane_constraints_particles, self.symmetry_planes =\
            self.params.strain_solid.get_fields("plane_constraints", "plane_constraints_particles", "symmetry_planes")

        initdims = self.params.initial_dimensions

        axis = self.params.axis

        otheraxis1 = (axis + 1) % 3
        otheraxis2 = (axis + 2) % 3

        # [constraint plane, 0: point on plane 1: normal, vector axis]
        self.plane_constraints[0, :, axis] = [0, 1]
        self.plane_constraints[1, :, otheraxis1] = [0, 1]
        self.plane_constraints[2, :, otheraxis2] = [0, 1]

        self.plane_constraints_particles[np.where(self.positions[:, axis] == 0)] |= 1 << 0
        self.plane_constraints_particles[np.where(self.positions[:, otheraxis1] == 0)] |= 1 << 1
        self.plane_constraints_particles[np.where(self.positions[:, otheraxis2] == 0)] |= 1 << 2

        self.apply_moving_constraint()

        self.symmetry_planes[0, :, 0] = [0, 1]
        self.symmetry_planes[1, :, 1] = [0, 1]
        self.symmetry_planes[2, :, 2] = [0, 1]

    def apply_moving_constraint(self):
        axis = self.params.axis
        initdims = self.params.initial_dimensions

        self.plane_constraints[3, :, axis] = [initdims[axis]/2, 1]
        self.plane_constraints_particles[self.positions[:, self.params.axis] == self.params.initial_dimensions[axis]/2] |= 1 << 3

    def set_plane_constraints(self, plane_position):
        self.plane_constraints[3, 0, self.params.axis] = plane_position

class StrainTestSymPadding(StrainTestSym):
    def __init__(self, **params):
        super(StrainTestSymPadding, self).__init__(**params)

    def init_positions(self):
        p = self.params

        initdims = p.initial_dimensions
        pdims = p.particle_dimensions

        # Generate particle positions
        x, y, z = np.meshgrid(np.linspace(0, initdims[0]/2, num=pdims[0]),
                              np.linspace(0, initdims[1]/2, num=pdims[1]),
                              np.linspace(0, initdims[2]/2, num=pdims[2]),
                              indexing='ij')

        self.positions = np.vstack((x.flatten(), y.flatten(), z.flatten())).T

        pspacing = [ i / 2.0 / (pd-1) for i, pd in zip(initdims, pdims) ]

        smoothingradius = p.strain_solid.get_fields("smoothingradius")[0]

        padding_number = [ math.ceil(smoothingradius / psp) for psp in pspacing ]
        self.padding_width = [ pn * psp for pn, psp in zip(padding_number, pspacing) ]
 
        px, py, pz = np.meshgrid(np.linspace(0, initdims[0]/2 + self.padding_width[0], num=(pdims[0]+padding_number[0])),
                                 np.linspace(0, initdims[1]/2 + self.padding_width[1], num=(pdims[1]+padding_number[1])),
                                 np.linspace(0, initdims[2]/2 + self.padding_width[2], num=(pdims[2]+padding_number[2])))

        padding = np.vstack((px.flatten(), py.flatten(), pz.flatten())).T
        padding = padding[np.logical_or(padding[:,0] > (initdims[0] + pspacing[0])/2,
                                        np.logical_or(padding[:,1] > (initdims[1] + pspacing[1])/2, padding[:,2] > (initdims[2] + pspacing[2])/2))]

        self.positions = np.concatenate((self.positions, padding))

    def post_init(self):
        super(StrainTestSymPadding, self).post_init()

        p = self.params

        initdims = p.initial_dimensions
        pdims = p.particle_dimensions

        pspacing = [ i / 2.0 / (pd-1) for i, pd in zip(initdims, pdims) ]

        axis = self.params.axis

        otheraxis1 = (axis + 1) % 3
        otheraxis2 = (axis + 2) % 3

        self.padding_field = self.params.strain_solid.get_fields("padding")[0]

        self.padding_field[np.where(self.positions[:,axis] > (initdims[axis] + pspacing[axis])/2)] |= 1 << 0
        self.padding_field[np.where(self.positions[:,otheraxis1] > (initdims[otheraxis1] + pspacing[otheraxis1])/2)] |= 1 << 1
        self.padding_field[np.where(self.positions[:,otheraxis2] > (initdims[otheraxis2] + pspacing[otheraxis2])/2)] |= 1 << 2

    def apply_moving_constraint(self):
        axis = self.params.axis
        initdims = self.params.initial_dimensions

        self.plane_constraints[3, :, axis] = [self.get_init_plane_position(), 1]
        self.plane_constraints_particles[np.where(self.positions[:, axis] == initdims[axis]/2 + self.padding_width[axis])] |= 1 << 3

    def get_init_plane_position(self):
        return self.params.initial_dimensions[self.params.axis] / 2 + self.padding_width[self.params.axis]
