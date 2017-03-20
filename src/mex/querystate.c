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

    char ** strarg = mxCalloc(nrhs, sizeof(char*));
    char ** strarg_mex = mxCalloc(nrhs, sizeof(char*));

    for (int i = 0; i < nrhs; ++i) {
        if (mxIsChar(prhs[i])) {
            size_t buflen = mxGetN(prhs[i])*sizeof(mxChar) + 1;
            strarg[i] = mxMalloc(buflen);
            mxGetString(prhs[i], strarg[i], buflen);

            size_t mexbuf_len;
            get_mex_field_name(strarg[i], &mexbuf_len, NULL);

            strarg_mex[i] = mxMalloc((mexbuf_len+1)*sizeof(char));
            get_mex_field_name(strarg[i], NULL, strarg_mex[i]);

            int j = get_host_field_psdata(data, strarg_mex[i]);

            if (j >= 0) plhs[i] = data->host_data[j];
            else note(2, "%s not found - skipped\n", strarg[i]);
        } else {
            mexErrMsgIdAndTxt("QueryState:ArgError", "Argument %d is not a string", i);
        }
    }

    sync_psdata_fields_device_to_host(*data, *pso, nrhs, (const char * const *) strarg);

    sync_to_mex(data);
    
    for (int i = 0; i < nrhs; ++i) {
        if (strarg[i] != NULL) {
            mxFree(strarg[i]);
        }

        if (strarg_mex[i] != NULL) {
            mxFree(strarg_mex[i]);
        }
    }

    mxFree(strarg);
    mxFree(strarg_mex);
}
