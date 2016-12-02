#include <string.h>
#include <stdio.h>

#include "mex.h"
#include "../config.h"
#include "../note.h"
#include "../particle_system.h"
#include "../opencl/particle_system_host.h"

static psdata * data = NULL;
static psdata_opencl pso;

void onExit() {
    free_psdata_opencl(&pso);
    terminate_opencl();

    free_stored_psdata();
}

void mexFunction(int nlhs, mxArray * plhs[], int nrhs, const mxArray * prhs[]) {
    if (nlhs != 0) mexErrMsgIdAndTxt("Test:outputerror", "This function does not return anything");

    psdata * data = get_stored_psdata();

    if (data != NULL) mexErrMsgIdAndTxt("Test:error", "Module already initialised");

    mexAtExit(onExit);

    /* Config file to use */
    load_config("/../../conf/fluid.conf");

    /* Create a psdata instance from fluid.conf and store it internally */
    create_stored_psdata_from_string(get_config_section("psdata_specification"));

    data = get_stored_psdata();

    /* Gather and store system data */
    init_opencl();

    /* Create GPU-side buffers for data and compiles kernels
     *
     * pso is a handle for the data and kernels, used in subsequent calls
     */
    pso = create_psdata_opencl(data, get_config_section("opencl_kernel_files"));

    /* Store pso so simstep can find it */
    opencl_use_buflist(pso);

    unload_config();

    /* Create some particles */
    populate_position_cuboid_device_opencl(pso, -1.0, -1.0, -1.0, 1.0, 1.0, 1.0, 8, 8, 8);

    /* Return the particles to the host so we can call kernels with the right amount */
    sync_psdata_device_to_host(*data, pso);
}
