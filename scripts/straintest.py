import numpy as np
import argparse
from .sph import bindings as bnd
import time
import ctypes

from functools import reduce

# Get column indices where x[:,i] is in the columns of y
def column_intersection_indices(x, y):
    return np.where(reduce(np.logical_or,
                           tuple(np.all(x==y[i,:], axis=1)
                                 for i in range(y.shape[0])) ))[0]

class StrainTest:
    def __init__(self, **params):
        required = ["strain_range", "axis", "strain_rate", "stabilisation_time",
                    "initial_dimensions", "particle_dimensions", "total_mass", "strain_solid",
                    "output_particles", "output_fields", "history_particles", "history_fields"]

        missing = [key for key in required if not key in params]

        if len(missing) > 0:
            raise Exception("Missing required fields: {}".format(missing))

        bnd.set_log_level(3)

        self.params = argparse.Namespace(**params)

        self.__init_positions()

        ps = self.params.strain_solid

        ps.setup(position=self.positions, n=self.positions.shape[0], mass=self.params.total_mass/np.prod(self.params.particle_dimensions))

        self.current_strain = 0;
        self.plane_frame_step = self.params.strain_rate * ps.get_fields("timestep")[0] * self.params.initial_dimensions[self.params.axis] / 2
        self.stabilisation_steps = int(self.params.stabilisation_time / ps.get_fields("timestep")[0])

        self.__init_output_and_history_indices()
        self.__init_plane_constraints()
        self.__init_run_path()
        self.__init_output_arrays()
        self.__init_history_arrays()

        ps.sync_host_to_device()

    def __init_positions(self):
        p = self.params

        initdims = p.initial_dimensions
        pdims = p.particle_dimensions

        # Generate particle positions
        x, y, z = np.meshgrid(np.linspace(-initdims[0]/2, initdims[0]/2, num=pdims[0]),
                            np.linspace(-initdims[1]/2, initdims[1]/2, num=pdims[1]),
                            np.linspace(-initdims[2]/2, initdims[2]/2, num=pdims[2]),
                            indexing='ij')

        self.positions = np.vstack((x.flatten(), y.flatten(), z.flatten())).T

    def __init_output_and_history_indices(self):
        pdims = self.params.particle_dimensions

        x, y, z = np.meshgrid(np.arange(pdims[0]), np.arange(pdims[1]), np.arange(pdims[2]), indexing='ij')

        indices = np.vstack((x.flatten(), y.flatten(), z.flatten())).T

        self.output_particle_indices = column_intersection_indices(indices, self.params.output_particles)

        if self.params.history_particles == "all":
            self.history_particle_indices = np.arange(self.positions.shape[0])
        else:
            self.history_particle_indices = column_intersection_indices(indices, self.history_particles)

    def __init_plane_constraints(self):
        # Constrain the planes normal to axis
        self.plane_constraints, self.plane_constraints_particles =\
            self.params.strain_solid.get_fields("plane_constraints", "plane_constraints_particles")

        initdims = self.params.initial_dimensions

        axis = self.params.axis

        # [constraint plane, 0: point on plane 1: normal, vector axis]
        self.plane_constraints[0, :, axis] = [-initdims[axis]/2, 1]
        self.plane_constraints[1, :, axis] = [initdims[axis]/2, 1]

        self.plane_constraints_particles[self.positions[:, axis] == -initdims[axis]/2] = 1
        self.plane_constraints_particles[self.positions[:, axis] == initdims[axis]/2] = 2

    def __init_run_path(self):
        self.run_path = []

        tension = []
        compression = []

        p = self.params

        tension_order = np.where(p.strain_range >= 0)[0]
        compression_order = np.flipud(np.where(p.strain_range < 0)[0])

        self.strain_order = np.concatenate((tension_order, compression_order))

        init_halfwidth = p.initial_dimensions[p.axis] / 2

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

    def __init_output_arrays(self):
        p = self.params

        prototype_output_arrays = p.strain_solid.get_fields(*p.output_fields)

        self.output = { p.output_fields[i]: np.zeros((p.strain_range.size, self.output_particle_indices.size) + prototype_output_arrays[i].shape[1:])
                        for i in range(len(p.output_fields)) }

    def __init_history_arrays(self):
        p = self.params

        prototype_history_arrays = p.strain_solid.get_fields(*p.history_fields)

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

    def run(self, **kwargs):
        p = self.params

        strip_index = 0

        union_history_position = list(set(p.output_fields + p.history_fields))

        for section in self.run_path:
            for strip in section:
                for plane_position in strip:
                    self.plane_constraints[0, 0, p.axis] = -plane_position
                    self.plane_constraints[1, 0, p.axis] = plane_position

                    p.strain_solid.sync_fields_host_to_device("plane_constraints")

                    p.strain_solid.call_for_all_particles("apply_plane_constraints")

                    p.strain_solid.update()

                    p.strain_solid.sync_fields_device_to_host(*union_history_position)

                    self.record_history()

                self.record_output(self.strain_order[strip_index])
                strip_index += 1

            p.strain_solid.reset()
            self.__init_plane_constraints()
