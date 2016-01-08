#include <CL/opencl.h>

typedef struct {
    cl_mem position_buf;
    cl_mem posnext_buf;
    cl_mem velocity_buf;
    cl_mem veleval_buf;
    cl_mem velnext_buf;
    cl_mem acceleration_buf;
    cl_mem force_buf;
    cl_mem density_buf;
    cl_mem volume_buf;
} psdata_bufList;

void init_ps_opencl();
psdata_bufList init_psdata_opencl();
void terminate_ps_opencl();
