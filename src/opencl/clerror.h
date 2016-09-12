#include <assert.h>
#include <CL/opencl.h>

#include "../note.h"

#ifndef MATLAB_MEX_FILE
    #define ASSERT(x) assert(x)
#else
    #include "mex.h"
    #define ASSERT(x) if (!(x)) mexErrMsgTxt(#x"\n")
#endif
#define HANDLE_CL_ERROR(x) { note(0, #x"\n"); cl_int e = x; if (e != CL_SUCCESS) printCLError(e); ASSERT(e == CL_SUCCESS); }

void printCLError(cl_int error);
void contextErrorCallback(const char * errinfo, const void * private_info,
                          size_t cb, void * user_data);
