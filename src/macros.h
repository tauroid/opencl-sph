#ifndef MACROS_H_
#define MACROS_H_

#include <assert.h>

#ifndef MATLAB_MEX_FILE
    #define ASSERT(x) assert(x)
#else
    #include "mex.h"
    #define ASSERT(x) if (!(x)) mexErrMsgTxt(#x"\n")
#endif

#endif
