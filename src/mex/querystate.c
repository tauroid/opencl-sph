#include <string.h>

#include "mex.h"

#include "../note.h"
#include "../particle_system.h"
#include "../opencl/particle_system_host.h"

void mexFunction(int nlhs, mxArray * plhs[], int nrhs, const mxArray * prhs[]) {
    psdata * data = get_stored_psdata();
    psdata_opencl * pso = get_stored_psdata_opencl();

    if (data == NULL || pso == NULL) mexErrMsgIdAndTxt("QueryState:InitError", "Module not initialised");
    if (nlhs < nrhs && nlhs > 1) mexErrMsgIdAndTxt("QueryState:ArgError", "Not enough return arguments");

    sync_psdata_device_to_host(data, *pso);
    sync_to_mex(data);

    char ** strarg = calloc(nrhs, sizeof(char*));

    int i = 0, j;
    for (; i < nrhs; ++i) {
        if (mxIsChar(prhs[i])) {
            size_t buflen = mxGetN(prhs[i])*sizeof(mxChar) + 1;
            strarg[i] = mxMalloc(buflen);
            mxGetString(prhs[i], strarg[i], buflen);

            for (j = 0; j < data->num_host_fields; ++j) {
                if (strcmp(data->host_names[j], strarg[i]) == 0) {
                    plhs[i] = data->host_data[j];
                    break;
                }
            }
        }
    }
    
    for (; i < nrhs; ++i) {
        if (strarg[i] != NULL) {
            mxFree(strarg[i]);
        }
    }

    free(strarg);
}
