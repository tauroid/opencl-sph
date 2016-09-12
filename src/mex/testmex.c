#include "mex.h"
#include <cmath>
#include <cstring>

void mexFunction(int nlhs, mxArray * plhs[], int nrhs, const mxArray * prhs[]) {
    if (nlhs > 1) mexErrMsgIdAndTxt("Test:outputerror", "This function returns one value only.");
    if (nrhs != 2) mexErrMsgIdAndTxt("Test:inputerror", "This function takes two arguments.");

    plhs[0] = mxCreateDoubleMatrix(3,2,mxREAL);
    double stuff[] = { 0.0, 4.0, 3.0, 2.0, 8.0, 10.0 };
    double * stuffptr = (double*)mxCalloc(6,sizeof(double));
    memcpy(stuffptr,stuff,6*sizeof(double));
    mxSetPr(plhs[0], stuffptr);
    stuffptr[1] = 5.5;
}
