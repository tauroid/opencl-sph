#include <assert.h>
#include <string.h>

#include "mex.h"
#include "../note.h"
#include "../particle_system.h"
#include "../opencl/particle_system_host.h"

static psdata * data = 0x0;
static psdata_bufList * bl = 0x0;

void onExit();

void mexFunction(int nlhs, mxArray * plhs[], int nrhs, const mxArray * prhs[]) {
    if (nlhs != 1) mexErrMsgIdAndTxt("Test:outputerror", "This function returns one value");
    set_log_level(0);

    note(2, "Started\n");

    if (data == 0x0) {
        data = mxMalloc(sizeof(psdata));
        mexMakeMemoryPersistent(data);

        init_psdata_fluid(data, 1000, 1, 0.02, 3, 0, 0, 0, 3, 3, 3);

        init_ps_opencl();

        psdata_bufList _bl = create_psdata_opencl(*data);
        
        bl = mxMalloc(sizeof(psdata_bufList));
        mexMakeMemoryPersistent(bl);

        memcpy(bl, &_bl, sizeof(psdata_bufList));
    }

    opencl_use_buflist(*bl);

    populate_position_cuboid_device_opencl(0, 0, 0, 2, 2, 2, 10, 10, 10);

    sync_psdata_device_to_host(*data, *bl);

    sync_to_mex(data);

    int p = get_host_field_psdata(data, "position_mex");
    assert(p != -1);

    plhs[0] = data->host_data[p];

    mexAtExit(onExit);
}

void onExit() {
    free_psdata_opencl(*bl);
    terminate_ps_opencl();

    free_psdata(*data);

    mxFree(data);
    mxFree(bl);
}
