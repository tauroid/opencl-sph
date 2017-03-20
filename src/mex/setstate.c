#include <string.h>

#include "mex.h"

#include "../particle_system.h"
#include "../opencl/particle_system_host.h"

void mexFunction(int nlhs, mxArray * plhs[], int nrhs, const mxArray * prhs[]) {
    psdata * data = get_stored_psdata();
    psdata_opencl * pso = get_stored_psdata_opencl();

    if (data == NULL || pso == NULL) mexErrMsgIdAndTxt("SetState:InitError", "Module not initialised");

    // Let's just skip the mex buffers entirely - they'll be updated on read
    
    char ** strarg = mxCalloc(nrhs/2, sizeof(char*));

    for (int i = 0; i+1 < nrhs; i += 2) {
        if (!mxIsChar(prhs[i])) {
            mexErrMsgIdAndTxt("SetState:ArgError", "Argument %d is not a string", i);
        }

        size_t buflen = mxGetN(prhs[i])*sizeof(mxChar) + 1;
        strarg[i/2] = mxMalloc(buflen);
        mxGetString(prhs[i], strarg[i/2], buflen);

        int j = get_field_psdata(*data, strarg[i/2]);

        if (j == -1) mexErrMsgIdAndTxt("SetState:ArgError", "Field \"%s\" was not found", strarg[i/2]);

        memcpy((char*) data->data + data->data_offsets[j], mxGetData(prhs[i+1]), mxGetElementSize(prhs[i+1])*mxGetNumberOfElements(prhs[i+1]));
    }

    sync_psdata_fields_host_to_device(*data, *pso, nrhs/2, (const char * const *) strarg);

    for (int i = 0; i < nrhs/2; ++i) {
        if (strarg[i] != NULL) mxFree(strarg[i]);
    }

    mxFree(strarg);
}
