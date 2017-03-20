#include "mex.h"

#include "../particle_system.h"
#include "../opencl/particle_system_host.h"

void mexFunction(int nlhs, mxArray * plhs[], int nrhs, const mxArray * prhs[]) {
    psdata * data = get_stored_psdata();
    psdata_opencl * pso = get_stored_psdata_opencl();

    if (data == NULL || pso == NULL) mexErrMsgIdAndTxt("ComputeBins:InitError", "Module not initialised");

    compute_particle_bins_device_opencl(*pso);
}
