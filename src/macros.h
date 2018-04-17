#ifndef MACROS_H_
#define MACROS_H_

#include <assert.h>

#ifndef MATLAB_MEX_FILE
    #define ASSERT(x) assert(x)
#else
    #include "mex.h"
    #define ASSERT(x) if (!(x)) mexErrMsgTxt(#x"\n")
#endif

#define STRINGIFY2(X) #X
#define STRINGIFY(X) STRINGIFY2(X)

#endif
