#include "mex.h"

#include "../particle_system.h"
#include "../opencl/particle_system_host.h"

void mexFunction(int nlhs, mxArray * plhs[], int nrhs, const mxArray * prhs[]) {
    psdata * data = get_stored_psdata();
    psdata_opencl * pso = get_stored_psdata_opencl();

    if (data == NULL || pso == NULL) mexErrMsgIdAndTxt("SyncAllHostToDevice:InitError", "Module not initialised");

    sync_psdata_host_to_device(*data, *pso, 0);
}
