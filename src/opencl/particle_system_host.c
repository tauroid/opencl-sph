#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <CL/opencl.h>
#include <assert.h>

#include "particle_system_host.h"
#include "../particle_system.h"
#include "../note.h"
#include "platforminfo.h"

#ifdef MATLAB_MEX_FILE
#include "mex.h"
#define malloc mxMalloc
#define free mxFree
#endif

static int ready = 0;
static cl_context context = NULL;
static cl_command_queue * command_queues;
static unsigned int num_command_queues = 0;

static int buffers_ready = 0;
static psdata_bufList bl;

void init_ps_opencl() {
    if (ready) return;
    
    Platform const * platforms;
    unsigned int num_platforms;

    get_opencl_platform_info(&platforms, &num_platforms);

    if (num_platforms == 0) return;

    const cl_context_properties context_properties[] = {
        CL_CONTEXT_PLATFORM, (cl_context_properties) platforms[0].id, 0
    };
    
    cl_int error;

    context = clCreateContextFromType
        ( (const cl_context_properties*) context_properties,
          CL_DEVICE_TYPE_GPU, NULL, NULL, &error );

    assert(error == CL_SUCCESS);

    command_queues     = malloc(platforms[0].num_devices*sizeof(cl_command_queue));
    num_command_queues = platforms[0].num_devices;

    const cl_queue_properties queue_properties[] = {
        CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE, 0
    };

    for (int i = 0; i < num_command_queues; ++i) {
        command_queues[i] = clCreateCommandQueue
            ( context, platforms[0].devices[i].id,
              CL_QUEUE_PROFILING_ENABLE, &error );

        assert(error == CL_SUCCESS);
    }
}

/* Only one psdata represented device-side for now */
psdata_bufList init_psdata_opencl(psdata * data) {
    if (buffers_ready) return bl;

    assert(num_command_queues > 0);

    cl_mem_flags flags = CL_MEM_USE_HOST_PTR;

    bl.position_buf     = clCreateBuffer(context, flags, POSITION_SIZE(data), data->position, NULL);
    bl.posnext_buf      = clCreateBuffer(context, flags, POSNEXT_SIZE(data), data->posnext, NULL);
    bl.velocity_buf     = clCreateBuffer(context, flags, VELOCITY_SIZE(data), data->velocity, NULL);
    bl.veleval_buf      = clCreateBuffer(context, flags, VELEVAL_SIZE(data), data->veleval, NULL);
    bl.velnext_buf      = clCreateBuffer(context, flags, VELNEXT_SIZE(data), data->velnext, NULL);
    bl.acceleration_buf = clCreateBuffer(context, flags, ACCELERATION_SIZE(data), data->acceleration, NULL);
    bl.force_buf        = clCreateBuffer(context, flags, FORCE_SIZE(data), data->force, NULL);
    bl.density_buf      = clCreateBuffer(context, flags, DENSITY_SIZE(data), data->density, NULL);
    bl.volume_buf       = clCreateBuffer(context, flags, VOLUME_SIZE(data), data->volume, NULL);

    buffers_ready = 1;

    return bl;
}

void free_psdata_opencl() {
    clReleaseMemObject(bl.position_buf);
    clReleaseMemObject(bl.posnext_buf);
    clReleaseMemObject(bl.velocity_buf);
    clReleaseMemObject(bl.veleval_buf);
    clReleaseMemObject(bl.velnext_buf);
    clReleaseMemObject(bl.acceleration_buf);
    clReleaseMemObject(bl.force_buf);
    clReleaseMemObject(bl.density_buf);
    clReleaseMemObject(bl.volume_buf);

    buffers_ready = 0;
}

void sync_psdata_host_to_device(psdata * data) {
    clEnqueueWriteBuffer(command_queues[0], bl.position_buf, CL_FALSE, 0,
                         POSITION_SIZE(data), data->position, 0, NULL, NULL);
    clEnqueueWriteBuffer(command_queues[0], bl.posnext_buf, CL_FALSE, 0,
                         POSNEXT_SIZE(data), data->posnext, 0, NULL, NULL);
    clEnqueueWriteBuffer(command_queues[0], bl.velocity_buf, CL_FALSE, 0, 
                         VELOCITY_SIZE(data), data->velocity, 0, NULL, NULL);
    clEnqueueWriteBuffer(command_queues[0], bl.veleval_buf, CL_FALSE, 0, 
                         VELEVAL_SIZE(data), data->veleval, 0, NULL, NULL);
    clEnqueueWriteBuffer(command_queues[0], bl.velnext_buf, CL_FALSE, 0,
                         VELNEXT_SIZE(data), data->position, 0, NULL, NULL);
    clEnqueueWriteBuffer(command_queues[0], bl.acceleration_buf, CL_FALSE, 0,
                         ACCELERATION_SIZE(data), data->acceleration, 0, NULL, NULL);
    clEnqueueWriteBuffer(command_queues[0], bl.density_buf, CL_FALSE, 0,
                         DENSITY_SIZE(data), data->density, 0, NULL, NULL);
    clEnqueueWriteBuffer(command_queues[0], bl.volume_buf, CL_FALSE, 0,
                         VOLUME_SIZE(data), data->volume, 0, NULL, NULL);

    clEnqueueBarrierWithWaitList(command_queues[0], 0, NULL, NULL);
}

void sync_psdata_device_to_host(psdata * data) {

}

void terminate_ps_opencl() {
    free_opencl_platform_info();

    for (int i = 0; i < num_command_queues; ++i) {
        clReleaseCommandQueue(command_queues[i]);
    }

    free(command_queues);

    clReleaseContext(context);

    printf("Done :)\n");
}
