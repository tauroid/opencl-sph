#include "mex.h"
#include "../particle_system.h"

void onExit();

void mexFunction(int nlhs, mxArray * plhs[], int nrhs, const mxArray * prhs[]) {
    if (nlhs != 1) mexErrMsgIdAndTxt("Test:outputerror", "This function returns one value");

    psdata * data = get_psdata_instance();
    if (data->pnum == 0) init_psdata(data, 27, 1, 0.02, 3);

    data->position[0] += 1;
    data->position[1] += 0.5;
    plhs[0] = data->position_mex;

    mexAtExit(onExit);
}

void onExit() {
    if (exists_psdata_instance()) {
        psdata * data = get_psdata_instance();
        free_psdata(data);
        free_psdata_instance();
    }
}
