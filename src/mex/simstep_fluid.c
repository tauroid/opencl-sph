#include <string.h>

#include "mex.h"

#include "../note.h"
#include "../particle_system.h"
#include "../opencl/particle_system_host.h"

static psdata * data = NULL;
static psdata_opencl * pso = NULL;

void mexFunction(int nlhs, mxArray * plhs[], int nrhs, const mxArray * prhs[]) {
    data = get_stored_psdata();
    pso = get_stored_psdata_opencl();

    if (data == NULL || pso == NULL) mexErrMsgIdAndTxt("Test:simsteperror", "Module not initialised");

    compute_particle_bins_device_opencl(*pso);

    call_for_all_particles_device_opencl(*pso, "compute_density");
    call_for_all_particles_device_opencl(*pso, "compute_forces_fluids");

    call_for_all_particles_device_opencl(*pso, "step_forward");
}
