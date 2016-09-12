#include <string.h>

#include "mex.h"
#include "../particle_system.h"
#include "../opencl/particle_system_host.h"

static psdata * data = NULL;

void mexFunction(int nlhs, mxArray * plhs[], int nrhs, const mxArray * prhs[]) {
    data = get_stored_psdata();

    if (data == NULL) mexErrMsgIdAndTxt("Test:simsteperror", "Module not initialised");

    note(2, "found a data at %p\n", data);

    unsigned int * n_ptr;

    PS_SET_PTR(data, "n", unsigned int, &n_ptr);

    unsigned int n = *n_ptr;

    note(2, "n is %u\n", n);

    bin_and_count_device_opencl(data);
    prefix_sum_device_opencl(data);
    copy_celloffset_to_backup_device_opencl(data);
    insert_particles_in_bin_array_device_opencl(data);

    compute_density_device_opencl(data);
    compute_forces_device_opencl(data);

    step_forward_device_opencl(data);
}
