#include "mex.h"

#include "../particle_system.h"
#include "../opencl/particle_system_host.h"

#define KERNEL_NAME_MAX 512

void mexFunction(int nlhs, mxArray * plhs[], int nrhs, const mxArray * prhs[]) {
    psdata * data = get_stored_psdata();
    psdata_opencl * pso = get_stored_psdata_opencl();

    if (data == NULL || pso == NULL) mexErrMsgIdAndTxt("CallKernel:InitError", "Module not initialised");

    char kernel_name_pad[KERNEL_NAME_MAX] = {0};

    for (int i = 0; i < nrhs; ++i) {
        if (!mxIsChar(prhs[i])) mexErrMsgIdAndTxt("CallKernel:ArgError", "Argument %d is not a string", i);
        size_t kernel_name_len = mxGetN(prhs[i]) + 1;
        mxGetString(prhs[i], kernel_name_pad, kernel_name_len);

        call_for_all_particles_device_opencl(*pso, kernel_name_pad);
    }
}
