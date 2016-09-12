#include "../opencl/particle_system_host.h"
#include "mex.h"

void mexFunction(int nlhs, mxArray * plhs[], int nrhs, const mxArray * prhs[]) {
    init_ps_opencl();
    terminate_ps_opencl();
}
