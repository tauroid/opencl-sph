#include <CL/opencl.h>
#ifdef MATLAB_MEX_FILE
#include "mex.h"
#define printf mexPrintf
#define malloc mxMalloc
#define free mxFree
#else
#include <stdio.h>
#endif

void printDeviceInfo(cl_device_id, cl_device_info);

void do_whatever() {
    cl_platform_id * platforms;
    cl_uint num_platforms;

    clGetPlatformIDs(0, NULL, &num_platforms);
    platforms = (cl_platform_id*) malloc(num_platforms*sizeof(cl_platform_id));
    clGetPlatformIDs(num_platforms, platforms, NULL);
    
    printf("%d platform%s present.\n\n", num_platforms, num_platforms == 1 ? "" : "s");

    int p,d;
    size_t platform_name_size;
    char * platform_name;
    cl_device_id * devices;
    cl_uint num_devices;
    for (p = 0; p < num_platforms; ++p) {
        clGetPlatformInfo(platforms[p], CL_PLATFORM_NAME, 0, NULL, &platform_name_size);
        platform_name = (char*) malloc(platform_name_size);
        clGetPlatformInfo(platforms[p], CL_PLATFORM_NAME, platform_name_size, platform_name, NULL);
        printf("%s\n", platform_name);
        free(platform_name);

        clGetDeviceIDs(platforms[p], 
                       CL_DEVICE_TYPE_ALL,
                       0,
                       NULL,
                       &num_devices);
        devices = (cl_device_id*) malloc(num_devices*sizeof(cl_device_id));
        clGetDeviceIDs(platforms[p],
                       CL_DEVICE_TYPE_ALL,
                       num_devices,
                       devices,
                       NULL);

        printf("%d devices on this platform:\n", num_devices);

        for (d = 0; d < num_devices; ++d) {
            printf("Name: "); printDeviceInfo(devices[d], CL_DEVICE_NAME); printf("\n");
            printf("Built-ins: ");
            printDeviceInfo(devices[d], CL_DEVICE_COMPILER_AVAILABLE);
            printf("\n\n");
        }
    }
}

void printDeviceInfo(cl_device_id device, cl_device_info param_name) {
    size_t device_info_size;
    char * device_info;

    clGetDeviceInfo(device, param_name, 0, NULL, &device_info_size);
    device_info = (char*) malloc(device_info_size);
    clGetDeviceInfo(device, param_name, device_info_size, device_info, NULL);
    printf("%s", device_info);
    free(device_info);
}    
