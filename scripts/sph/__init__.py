from . import bindings as bnd
import ctypes
import atexit

def init_opencl():
    bnd.init_opencl()

def terminate_opencl():
    bnd.terminate_opencl()

def init_data(config_file):
    if bnd.load_config(config_file) > 0:
        print("Initialization failed: could not read config file {}".format(config_file))
        return

    psdata = init_data_cpu(bnd.get_config_section("psdata_specification"))

    psdata_opencl = init_data_opencl(psdata, bnd.get_config_section("opencl_kernel_files"))

    bnd.unload_config()

    return psdata, psdata_opencl

def free_data(psdata, psdata_opencl):
    free_data_cpu(psdata)
    free_data_opencl(psdata_opencl)

def init_data_cpu(config_spec_string):
    psdata = bnd.psdata()
    bnd.build_psdata_from_string(ctypes.byref(psdata), config_spec_string);

    return psdata

def free_data_cpu(psdata):
    bnd.free_psdata(ctypes.byref(psdata))

def free_data_opencl(psdata_opencl):
    bnd.free_psdata_opencl(ctypes.byref(psdata_opencl))

def init_data_opencl(psdata, config_kernels):
    psdata_opencl = bnd.create_psdata_opencl(ctypes.byref(psdata), config_kernels)

    return psdata_opencl
