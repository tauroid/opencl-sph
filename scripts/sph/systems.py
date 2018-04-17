from . import bindings as bnd
import numpy as np
import ctypes
import os.path

from . import init_data, free_data

class ParticleSystem(object):
    config = None
    numpy_types = {
        bnd.T_CHAR:    np.dtype('S'),
        bnd.T_UINT:    np.dtype('u4'),
        bnd.T_INT:     np.dtype('i4'),
        bnd.T_FLOAT32: np.dtype('f4'),
        bnd.T_FLOAT64: np.dtype('f8')
    }

    union_types = {
        bnd.T_CHAR:    'c',
        bnd.T_UINT:    'u',
        bnd.T_INT:     'i',
        bnd.T_FLOAT32: 'f4',
        bnd.T_FLOAT64: 'f8'
    }

    def __init__(self):
        self.psdata, self.psdata_opencl = init_data(self.config)

        self.fields = {}

    def is_numpy_array_correct_type(self, array, field_name):
        return array.dtype == self.numpy_types[bnd]

    def get_fields(self, *args):
        fields = []
        for arg in args:
            if not arg in self.fields:
                field_index = bnd.get_field_psdata(self.psdata, arg)
                if field_index == -1:
                    raise Exception("{} is not a field.".format(arg))

                field_ptr = self.psdata.data_ptrs[field_index];

                dimensions_ptr = ctypes.cast(self.psdata.dimensions, ctypes.c_voidp).value
                dimensions_ptr += self.psdata.dimensions_offsets[field_index] * ctypes.sizeof(self.psdata.dimensions.contents)
                dimensions_ptr = ctypes.cast(dimensions_ptr, type(self.psdata.dimensions))

                dimensions = np.ctypeslib.as_array(dimensions_ptr, shape=(self.psdata.num_dimensions[field_index], ))

                new_array = np.ctypeslib.as_array (
                        getattr(self.psdata.data_ptrs[field_index],
                                self.union_types[field_ptr.type]),
                        shape = tuple(np.flipud(dimensions))
                        )

                self.fields[arg] = new_array

            fields.append(self.fields[arg])

        return tuple(fields)

    def set_fields(self, *args, **kwargs):
        if len(args) > 0 and args[0]:
            host_only = True
        else:
            host_only = False

        argkeys = kwargs.keys()
        argvals = [kwargs[key] for key in argkeys]

        fields = self.get_fields(*argkeys)

        for i in range(len(argvals)):
            if type(argvals[i]) == np.ndarray and argvals[i].size > 1:
                valshape = argvals[i].shape
                np.copyto(fields[i][[slice(None,s) for s in valshape]], argvals[i])
            else:
                np.copyto(fields[i], argvals[i])

        if not host_only:
            self.sync_fields_host_to_device(*argkeys)

    def sync_host_to_device(self):
        bnd.sync_psdata_host_to_device(self.psdata, self.psdata_opencl, 0)

    def sync_device_to_host(self):
        bnd.sync_psdata_device_to_host(self.psdata, self.psdata_opencl)

    def sync_fields_host_to_device(self, *args):
        fields_ptr = (ctypes.c_char_p * len(args))()
        fields_ptr[:] = [arg.encode('utf-8') for arg in args]

        bnd.sync_psdata_host_to_device(self.psdata, self.psdata_opencl, len(args), fields_ptr)

    def sync_fields_device_to_host(self, *args):
        fields_ptr = (ctypes.c_char_p * len(args))()
        fields_ptr[:] = [arg.encode('utf-8') for arg in args]

        bnd.sync_psdata_device_to_host(self.psdata, self.psdata_opencl, len(args), fields_ptr)
    
    def set_body_position(self, body, position):
        bnd.set_body_position_device_opencl(self.psdata_opencl, body, position.astype(np.dtype('f4')))

    def set_body_rotation(self, body, euler_angles):
        bnd.set_body_rotation_device_opencl(self.psdata_opencl, body, euler_angles.astype(np.dtype('f4')))

    def call_for_all_particles(self, kernel):
        bnd.call_for_all_particles_device_opencl(self.psdata_opencl, kernel)

    def init(self, **kwargs):
        self.set_fields(**kwargs)

    def update(self):
        if not self.psdata or not self.psdata_opencl:
            self.setup()

    def free(self):
        free_data(self.psdata, self.psdata_opencl)

        self.__init__()


class StrainSolid(ParticleSystem):
    config = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))) + "/conf/strainsolid.conf"
    init_values = None

    # Set values
    def init(self, **kwargs):
        super(StrainSolid, self).init(**kwargs)

        if self.init_values is None:
            # Store for later reset
            self.init_values = {}

            for field in kwargs:
                self.init_values[field] = np.copy(kwargs[field])

    # Pre-sim computation
    def setup(self):
        self.call_for_all_particles("init_original_position")

        bnd.compute_particle_bins_device_opencl(self.psdata_opencl)

        self.call_for_all_particles("compute_original_density")

        self.sync_device_to_host()

    def update(self):
        super(StrainSolid, self).update()

        bnd.compute_particle_bins_device_opencl(self.psdata_opencl)

        self.call_for_all_particles("compute_density")
        self.call_for_all_particles("compute_rotations_and_strains")
        self.call_for_all_particles("compute_stresses")
        self.call_for_all_particles("compute_forces_solids")

        self.call_for_all_particles("step_forward")

    # Reset to state after init, before setup
    def reset(self):
        free_data(self.psdata, self.psdata_opencl)

        self.fields = {}

        self.psdata, self.psdata_opencl = init_data(self.config)

        self.init(**self.init_values)

    def free(self):
        super(StrainSolid, self).free()

        self.init_values = None

class SVKSolid(StrainSolid):
    config = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))) + "/conf/svksolid.conf"
    init_values = None

    def update(self):
        super(StrainSolid, self).update()

        bnd.compute_particle_bins_device_opencl(self.psdata_opencl)

        self.call_for_all_particles("compute_density")
        self.call_for_all_particles("compute_deformations")
        self.call_for_all_particles("compute_stresses")
        self.call_for_all_particles("compute_forces_solids")

        self.call_for_all_particles("step_forward")
