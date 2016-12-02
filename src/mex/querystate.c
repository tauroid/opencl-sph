#include <string.h>
#include <stdio.h>

#include "mex.h"

#include "../note.h"
#include "../particle_system.h"
#include "../opencl/particle_system_host.h"

void mexFunction(int nlhs, mxArray * plhs[], int nrhs, const mxArray * prhs[]) {
    psdata * data = get_stored_psdata();
    psdata_opencl * pso = get_stored_psdata_opencl();

    if (data == NULL || pso == NULL) mexErrMsgIdAndTxt("QueryState:InitError", "Module not initialised");
    if (nlhs < nrhs && nlhs > 1) mexErrMsgIdAndTxt("QueryState:ArgError", "Not enough return arguments");

    char ** strarg = calloc(nrhs, sizeof(char*));
    char ** strarg_mex = calloc(nrhs, sizeof(char*));

    const char * mext = "_mex";
    size_t mext_len = strlen(mext) * sizeof(char);

    int i = 0;
    for (; i < nrhs; ++i) {
        if (mxIsChar(prhs[i])) {
            size_t buflen = mxGetN(prhs[i])*sizeof(mxChar) + 1;
            strarg[i] = mxMalloc(buflen);
            mxGetString(prhs[i], strarg[i], buflen);

            strarg_mex[i] = malloc(buflen+mext_len);
            sprintf(strarg_mex[i], "%s%s", strarg[i], mext);

            for (size_t j = 0; j < data->num_host_fields; ++j) {
                if (strcmp(data->host_names[j], strarg_mex[i]) == 0) {
                    plhs[i] = data->host_data[j];
                    break;
                }
            }
        } else {
            mexErrMsgIdAndTxt("QueryState:ArgError", "Argument %d is not a string", i);
        }
    }

    sync_psdata_fields_device_to_host(*data, *pso, nrhs, (const char * const *) strarg);

    sync_to_mex(data);
    
    for (; i < nrhs; ++i) {
        if (strarg[i] != NULL) {
            mxFree(strarg[i]);
        }

        if (strarg_mex[i] != NULL) {
            free(strarg_mex[i]);
        }
    }

    free(strarg);
}
