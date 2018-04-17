#pragma once

#ifndef OPENCL_SPH_REAL_TYPE
#define OPENCL_SPH_REAL_TYPE float
#endif

#include <CL/opencl.h>

#if OPENCL_SPH_REAL_TYPE == float
typedef cl_float2 REAL2;
typedef cl_float3 REAL3;
typedef cl_float4 REAL4;
#elif OPENCL_SPH_REAL_TYPE == double
typedef cl_double2 REAL2;
typedef cl_double3 REAL3;
typedef cl_double4 REAL4;
#else
#error "OPENCL_SPH_REAL_TYPE must be either float or double."
#endif

typedef OPENCL_SPH_REAL_TYPE REAL;
