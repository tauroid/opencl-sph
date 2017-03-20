from . import bindings as bnd
import numpy as np
import ctypes
import os.path

from . import init_data, free_data

class ParticleSystem(object):
    config = None

    def __init__(self):
        self.psdata = None
        self.psdata_opencl = None

        self.numpy_types = {}

        self.numpy_types[bnd.T_CHAR]    = np.dtype('S')
        self.numpy_types[bnd.T_UINT]    = np.dtype('u4')
        self.numpy_types[bnd.T_INT]     = np.dtype('i4')
        self.numpy_types[bnd.T_FLOAT32] = np.dtype('f4')
        self.numpy_types[bnd.T_FLOAT64] = np.dtype('f8')

        self.union_types = {}

        self.union_types[bnd.T_CHAR]    = 'c';
        self.union_types[bnd.T_UINT]    = 'u';
        self.union_types[bnd.T_INT]     = 'i';
        self.union_types[bnd.T_FLOAT32] = 'f4';
        self.union_types[bnd.T_FLOAT64] = 'f8';

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

    def setup(self, **kwargs):
        self.psdata, self.psdata_opencl = init_data(self.config)

        self.set_fields(**kwargs)

    def update(self):
        if not self.psdata or not self.psdata_opencl:
            self.setup()

class StrainSolid(ParticleSystem):
    config = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))) + "/conf/strainsolid.conf"
    required = ["position", "mass", "n"]
    setup_values = None

    def setup(self, **kwargs):
        if not all([key in kwargs for key in self.required]):
            raise Exception("Some required fields missing (required are {})".format(self.required))

        super(StrainSolid, self).setup(**kwargs)

        if self.setup_values is None:
            # Store for later reset
            self.setup_values = {}

            for field in kwargs:
                self.setup_values[field] = np.copy(kwargs[field])

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

    def reset(self):
        free_data(self.psdata, self.psdata_opencl)

        self.fields = {}

        self.setup(**self.setup_values)
