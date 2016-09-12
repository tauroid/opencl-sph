#include <assert.h>
#include <string.h>

#include "mex.h"
#include "../note.h"
#include "../particle_system.h"
#include "../opencl/particle_system_host.h"

static psdata * data = NULL;
static psdata_opencl pso;

void onExit() {
    note(2, "wtf already\n");
    free_psdata_opencl(pso);
    terminate_ps_opencl();

    free_stored_psdata();
}

void mexFunction(int nlhs, mxArray * plhs[], int nrhs, const mxArray * prhs[]) {
    if (nlhs != 0) mexErrMsgIdAndTxt("Test:outputerror", "This function does not return anything");

    data = get_stored_psdata();

    if (data != NULL) mexErrMsgIdAndTxt("Test:error", "Module already initialised");

    mexAtExit(onExit);

    char * exe_path = getenv("EXE_PATH");

    const char * kern_rel_path = "/../../conf/fluid.conf";

    char * kern_path = malloc((strlen(exe_path)+strlen(kern_rel_path)+1)*sizeof(char));
    sprintf(kern_path, "%s%s", exe_path, kern_rel_path);

    data = create_stored_psdata_from_conf(kern_path);

    free(kern_path);

    init_ps_opencl();
    pso = create_psdata_opencl(data);
    opencl_use_buflist(pso);

    populate_position_cuboid_device_opencl(0, 0, 0, 3, 3, 3, 10, 10, 10);

    sync_psdata_device_to_host(data, pso);

    unsigned int * n_ptr;

    PS_SET_PTR(data, "n", unsigned int, &n_ptr);

    unsigned int n = *n_ptr;

    note(2, "n = %u\n", n);

    note(2, "Data at %p\n", data);
}
