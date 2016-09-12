#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <CL/opencl.h>

#include "clerror.h"
#include "particle_system_host.h"
#include "../particle_system.h"
#include "../note.h"
#include "../3rdparty/whereami.h"
#include "platforminfo.h"

#define NUM_PS_ARGS 10

#define WG_FUJ_SZ 896

static int ready = 0;
static cl_context context = NULL;
static cl_command_queue * command_queues;
static unsigned int num_command_queues = 0;

static Platform const * platforms;
static unsigned int num_platforms;

static psdata_opencl pso;

char * add_field_macros_to_start_of_string(const char * string, psdata * data);

#ifdef MATLAB_MEX_FILE
psdata_opencl get_stored_psdata_opencl() {
    return pso;
}
#endif

/**
 * Initialise OpenCL
 *
 * Detects hardware, creates command queues
 */
void init_ps_opencl() {
    if (ready) return;

    get_opencl_platform_info(&platforms, &num_platforms);

    ASSERT(num_platforms > 0);

    const cl_context_properties context_properties[] = {
        CL_CONTEXT_PLATFORM, (cl_context_properties) platforms[0].id, 0
    };
    
    cl_int error;

    context = clCreateContextFromType
        ( (const cl_context_properties*) context_properties,
          CL_DEVICE_TYPE_GPU, contextErrorCallback, NULL, &error );

    HANDLE_CL_ERROR(error);

    command_queues     = malloc(platforms[0].num_devices*sizeof(cl_command_queue));
    num_command_queues = platforms[0].num_devices;

    ASSERT(num_command_queues > 0);

    const cl_queue_properties queue_properties[] = {
        CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE, 0
    };

    int i;
    for (i = 0; i < num_command_queues; ++i) {
        command_queues[i] = clCreateCommandQueue
            ( context, platforms[0].devices[i].id,
              CL_QUEUE_PROFILING_ENABLE, &error );

        HANDLE_CL_ERROR(error);
    }

    ready = 1;
}

void build_program(psdata * data, psdata_opencl * bl) {
    cl_int error;

    /* Get file data */
    {
        char * exe_path;

#ifndef MATLAB_MEX_FILE
        int exe_path_len;

        exe_path_len = wai_getExecutablePath(NULL, 0, NULL);
        exe_path = malloc((exe_path_len+1)*sizeof(char));
        wai_getExecutablePath(exe_path, exe_path_len, NULL);

        exe_path[exe_path_len] = 0x0;
        char * lastslash = strrchr(exe_path, '/');

        if (lastslash != NULL) {
            exe_path_len = (int)( (lastslash - exe_path) / sizeof(char) );
            exe_path[exe_path_len] = 0x0;
        }
#else
        exe_path = getenv("EXE_PATH");
#endif

#ifndef MATLAB_MEX_FILE
        const char * kern_rel_path = "/kernels/particle_system_kern.cl";
#else
        const char * kern_rel_path = "/../kernels/particle_system_kern.cl";
#endif

        char * kern_path = malloc((strlen(exe_path)+strlen(kern_rel_path)+1)*sizeof(char));

        sprintf(kern_path, "%s%s", exe_path, kern_rel_path);

#ifndef MATLAB_MEX_FILE
        free(exe_path);
#endif

        note(1, "Compiling OpenCL program at %s\n", kern_path);

        long int kern_string_len;
        char * kern_string;

        FILE * f = fopen(kern_path, "r");

        free(kern_path);

        ASSERT(f != NULL);

        fseek(f, 0, SEEK_END);
        kern_string_len = ftell(f);
        fseek(f, 0, SEEK_SET);

        kern_string = malloc((kern_string_len+1)*sizeof(char));
        fread(kern_string, sizeof(char), kern_string_len, f);

        fclose(f);

        kern_string[kern_string_len] = '\0';

        char * kern_string_with_macros = add_field_macros_to_start_of_string(kern_string, data);
        free(kern_string);

        size_t ks_macros_length = strlen(kern_string_with_macros);

        bl->ps_prog = clCreateProgramWithSource(context, 1, (const char **) &kern_string_with_macros, &ks_macros_length, &error);
        HANDLE_CL_ERROR(error);

        free(kern_string_with_macros);
    }

    cl_int build_error = clBuildProgram(bl->ps_prog, 1, &platforms[0].devices[0].id, "-cl-fast-relaxed-math", NULL, NULL);

    if (build_error != CL_SUCCESS) {
        char * error_log;
        size_t log_length;

        HANDLE_CL_ERROR(clGetProgramBuildInfo(bl->ps_prog, platforms[0].devices[0].id,
                                              CL_PROGRAM_BUILD_LOG, 0, NULL, &log_length));

        error_log = malloc(log_length*sizeof(char));

        HANDLE_CL_ERROR(clGetProgramBuildInfo(bl->ps_prog, platforms[0].devices[0].id,
                                              CL_PROGRAM_BUILD_LOG, log_length, error_log, NULL));

        printf("%s\n", error_log);

        free(error_log);

        HANDLE_CL_ERROR(build_error);
    }

    bl->kern_cuboid = clCreateKernel(bl->ps_prog, "populate_position_cuboid", &error); HANDLE_CL_ERROR(error);
    bl->kern_find_particle_bins = clCreateKernel(bl->ps_prog, "find_particle_bins", &error); HANDLE_CL_ERROR(error);
    bl->kern_count_particles_in_bins = clCreateKernel(bl->ps_prog, "count_particles_in_bins", &error); HANDLE_CL_ERROR(error);
    bl->kern_prefix_sum = clCreateKernel(bl->ps_prog, "prefix_sum", &error); HANDLE_CL_ERROR(error);
    bl->kern_copy_celloffset_to_backup = clCreateKernel(bl->ps_prog, "copy_celloffset_to_backup", &error); HANDLE_CL_ERROR(error);
    bl->kern_insert_particles_in_bin_array = clCreateKernel(bl->ps_prog, "insert_particles_in_bin_array", &error); HANDLE_CL_ERROR(error);

    bl->kern_compute_density = clCreateKernel(bl->ps_prog, "compute_density", &error); HANDLE_CL_ERROR(error);
    bl->kern_compute_forces = clCreateKernel(bl->ps_prog, "compute_forces", &error); HANDLE_CL_ERROR(error);
    bl->kern_step_forward = clCreateKernel(bl->ps_prog, "step_forward", &error); HANDLE_CL_ERROR(error);
}

/* Allocates new string */
char * add_field_macros_to_start_of_string(const char * string, psdata * data) {
    const char * start = "#define ";
    const char * middle = " (((global char *) data) + data_offsets[";
    const char * end = "])\n";

    size_t start_size = strlen(start);
    size_t middle_size = strlen(middle);
    size_t end_size = strlen(end);

    size_t string_size = 0;

    int f = 0;
    for (; f < data->num_fields; ++f) {
        string_size += start_size + strlen(data->names + data->names_offsets[f]) + 2 + middle_size + (int)(f == 0 ? 1 : floor(log10(f)) + 1) + end_size;
    }

    char * newstring = malloc(strlen(string) + string_size + 1);

    char * newstring_ptr = newstring;

    for (f = 0; f < data->num_fields; ++f) {
        strcpy(newstring_ptr, start); newstring_ptr += start_size;
        strcpy(newstring_ptr, data->names + data->names_offsets[f]); newstring_ptr += strlen(data->names + data->names_offsets[f]);
        strcpy(newstring_ptr, "_m"); newstring_ptr += 2;
        strcpy(newstring_ptr, middle); newstring_ptr += middle_size;
        sprintf(newstring_ptr, "%d", f); newstring_ptr += (int)(f == 0 ? 1 : floor(log10(f)) + 1);
        strcpy(newstring_ptr, end); newstring_ptr += end_size;
    }

    strcpy(newstring_ptr, string);

    return newstring;
}

void bin_and_count_device_opencl(psdata * data) {
    size_t max_workgroup_size = platforms[0].devices[0].max_workgroup_size;

    unsigned int * pnum_ptr;

    PS_SET_PTR(data, "pnum", unsigned int, &pnum_ptr)
    unsigned int pnum = *pnum_ptr;

    size_t num_p_workgroups = pnum / max_workgroup_size + 1;
    size_t num_p_workitems = num_p_workgroups * max_workgroup_size;

    HANDLE_CL_ERROR(clEnqueueNDRangeKernel(command_queues[0], pso.kern_find_particle_bins, 1,
                                           NULL, &num_p_workitems, &max_workgroup_size, 0, NULL, NULL));

    HANDLE_CL_ERROR(clFinish(command_queues[0]));

    HANDLE_CL_ERROR(clEnqueueNDRangeKernel(command_queues[0], pso.kern_count_particles_in_bins, 1,
                                           NULL, &num_p_workitems, &max_workgroup_size, 0, NULL, NULL));

    HANDLE_CL_ERROR(clFinish(command_queues[0]));
}

void prefix_sum_device_opencl(psdata * data) {
    HANDLE_CL_ERROR(clSetKernelArg(pso.kern_prefix_sum, NUM_PS_ARGS, 2*pso.po2_workgroup_size*sizeof(unsigned int), NULL));
    HANDLE_CL_ERROR(clSetKernelArg(pso.kern_prefix_sum, NUM_PS_ARGS + 1, sizeof(unsigned int), &pso.num_grid_cells));
    HANDLE_CL_ERROR(clSetKernelArg(pso.kern_prefix_sum, NUM_PS_ARGS + 2, sizeof(cl_mem), &pso.block_totals));
    HANDLE_CL_ERROR(clSetKernelArg(pso.kern_prefix_sum, NUM_PS_ARGS + 3, sizeof(unsigned int), &pso.num_blocks));

    size_t num_work_groups = pso.num_blocks;
    size_t num_work_items = num_work_groups * pso.po2_workgroup_size;

    HANDLE_CL_ERROR(clEnqueueNDRangeKernel(command_queues[0], pso.kern_prefix_sum, 1, NULL,
                                               &num_work_items, &pso.po2_workgroup_size, 0, NULL, NULL));

    HANDLE_CL_ERROR(clFinish(command_queues[0]));
}

void copy_celloffset_to_backup_device_opencl(psdata * data) {
    HANDLE_CL_ERROR(clSetKernelArg(pso.kern_copy_celloffset_to_backup, NUM_PS_ARGS, sizeof(cl_mem), &pso.backup_prefix_sum));
    HANDLE_CL_ERROR(clSetKernelArg(pso.kern_copy_celloffset_to_backup, NUM_PS_ARGS+1, sizeof(unsigned int), &pso.num_grid_cells));

    size_t num_work_items = pso.num_blocks * 2 * pso.po2_workgroup_size;

    HANDLE_CL_ERROR(clEnqueueNDRangeKernel(command_queues[0], pso.kern_copy_celloffset_to_backup, 1, NULL, &num_work_items, &pso.po2_workgroup_size, 0, NULL, NULL));

    HANDLE_CL_ERROR(clFinish(command_queues[0]));
}

void insert_particles_in_bin_array_device_opencl(psdata * data) {
    int * pnum_ptr;

    PS_SET_PTR(data, "pnum", int, &pnum_ptr);
    int n = pnum_ptr[1];

    size_t num_workgroups = n / pso.po2_workgroup_size + 1;
    size_t num_work_items = num_workgroups * pso.po2_workgroup_size;

    HANDLE_CL_ERROR(clSetKernelArg(pso.kern_insert_particles_in_bin_array, NUM_PS_ARGS, sizeof(cl_mem), &pso.backup_prefix_sum));

    HANDLE_CL_ERROR(clEnqueueNDRangeKernel(command_queues[0], pso.kern_insert_particles_in_bin_array, 1, NULL, &num_work_items, &pso.po2_workgroup_size, 0, NULL, NULL));
    
    HANDLE_CL_ERROR(clFinish(command_queues[0]));
}

void find_particle_bins_device_opencl(psdata * data) {
    size_t max_workgroup_size = WG_FUJ_SZ;

    unsigned int pn = get_field_psdata(data, "pnum");
    assert(pn != -1);

    unsigned int pnum = *((unsigned int *) ((char*) data->data + data->data_offsets[pn]));

    size_t num_workgroups = pnum / max_workgroup_size + 1;
    size_t num_workitems = num_workgroups * max_workgroup_size;

    HANDLE_CL_ERROR(clEnqueueNDRangeKernel(command_queues[0], pso.kern_find_particle_bins, 1,
                                           NULL, &num_workitems, &max_workgroup_size, 0, NULL, NULL));

    HANDLE_CL_ERROR(clFinish(command_queues[0]));
}

void count_particles_in_bins_device_opencl(psdata * data) {
    unsigned int max_workgroup_size = WG_FUJ_SZ;

}

void compute_density_device_opencl(psdata * data) {
    unsigned int * n_ptr;

    PS_SET_PTR(data, "n", unsigned int, &n_ptr);
    unsigned int n = *n_ptr;

    note(2, "n = %d\n", n);

    size_t num_workgroups = n / pso.po2_workgroup_size + 1;
    size_t num_workitems = num_workgroups * pso.po2_workgroup_size;

    note(2, "Starting density comp with %u workitems, workgroup size %d\n", num_workitems, pso.po2_workgroup_size);
    HANDLE_CL_ERROR(clEnqueueNDRangeKernel(command_queues[0], pso.kern_compute_density, 1, NULL,
                                           &num_workitems, &pso.po2_workgroup_size, 0, NULL, NULL));
    note(2, "Finished\n");

    HANDLE_CL_ERROR(clFinish(command_queues[0]));
}

void compute_forces_device_opencl(psdata * data) {
    unsigned int * n_ptr;

    PS_SET_PTR(data, "n", unsigned int, &n_ptr);
    unsigned int n = *n_ptr;

    size_t num_workgroups = n / pso.po2_workgroup_size + 1;
    size_t num_workitems = num_workgroups * pso.po2_workgroup_size;
    
    HANDLE_CL_ERROR(clEnqueueNDRangeKernel(command_queues[0], pso.kern_compute_forces, 1, NULL,
                                           &num_workitems, &pso.po2_workgroup_size, 0, NULL, NULL));

    HANDLE_CL_ERROR(clFinish(command_queues[0]));
}

void step_forward_device_opencl(psdata * data) {
    unsigned int * n_ptr;

    PS_SET_PTR(data, "n", unsigned int, &n_ptr);
    unsigned int n = *n_ptr;

    size_t num_workgroups = n / pso.po2_workgroup_size + 1;
    size_t num_workitems = num_workgroups * pso.po2_workgroup_size;
    
    HANDLE_CL_ERROR(clEnqueueNDRangeKernel(command_queues[0], pso.kern_step_forward, 1, NULL,
                                           &num_workitems, &pso.po2_workgroup_size, 0, NULL, NULL));

    HANDLE_CL_ERROR(clFinish(command_queues[0]));
}

void populate_position_cuboid_device_opencl(double x1, double y1, double z1,
                                            double x2, double y2, double z2,
                                            unsigned int xsize,
                                            unsigned int ysize,
                                            unsigned int zsize)
{
    size_t work_group_edge = (size_t) pow
        ((double) platforms[0].devices[0].max_workgroup_size, 1.0/3.0);
    size_t local_work_size[] = { work_group_edge, work_group_edge, work_group_edge };
    size_t global_work_size[] = { (xsize/work_group_edge + 1) * work_group_edge,
                                  (ysize/work_group_edge + 1) * work_group_edge,
                                  (zsize/work_group_edge + 1) * work_group_edge };

    cl_double3 corner1 = {{ x1, y1, z1 }};
    cl_double3 corner2 = {{ x2, y2, z2 }};
    cl_uint3 size = {{ xsize, ysize, zsize }};

    HANDLE_CL_ERROR(clSetKernelArg(pso.kern_cuboid, NUM_PS_ARGS, sizeof(cl_double3), &corner1));
    HANDLE_CL_ERROR(clSetKernelArg(pso.kern_cuboid, NUM_PS_ARGS+1, sizeof(cl_double3), &corner2));
    HANDLE_CL_ERROR(clSetKernelArg(pso.kern_cuboid, NUM_PS_ARGS+2, sizeof(cl_uint3), &size));

    HANDLE_CL_ERROR(clEnqueueNDRangeKernel(command_queues[0], pso.kern_cuboid, 3, NULL,
                                           global_work_size, local_work_size,
                                           0, NULL, NULL));
    HANDLE_CL_ERROR(clFinish(command_queues[0]));
}

/**
 * Create a mirror buffer for psdata on the GPU
 *
 * Copies the data from the supplied psdata struct to the GPU,
 * omitting host_data.
 *
 * @param data The host buffer to copy
 * @return Buffer list referencing the newly created device side buffer
 */
psdata_opencl create_psdata_opencl(psdata * data) {
    psdata_opencl bl;

    cl_mem_flags flags = CL_MEM_COPY_HOST_PTR;

    cl_int error;

    unsigned int nf = data->num_fields;

    unsigned int fmsize = nf*sizeof(unsigned int);

    bl.num_fields = nf;
    bl.names = clCreateBuffer(context, flags, psdata_names_size(data), (char*) data->names, &error);
    HANDLE_CL_ERROR(error);
    bl.names_offsets = clCreateBuffer(context, flags, fmsize, data->names_offsets, &error);
    HANDLE_CL_ERROR(error);
    bl.dimensions = clCreateBuffer(context, flags, psdata_dimensions_size(data), data->dimensions, &error);
    HANDLE_CL_ERROR(error);
    bl.num_dimensions = clCreateBuffer(context, flags, fmsize, data->num_dimensions, &error);
    HANDLE_CL_ERROR(error);
    bl.dimensions_offsets = clCreateBuffer(context, flags, fmsize, data->dimensions_offsets, &error);
    HANDLE_CL_ERROR(error);
    bl.entry_sizes = clCreateBuffer(context, flags, fmsize, data->entry_sizes, &error);
    HANDLE_CL_ERROR(error);
    bl.data = clCreateBuffer(context, flags, psdata_data_size(data), data->data, &error);
    HANDLE_CL_ERROR(error);
    bl.data_sizes = clCreateBuffer(context, flags, fmsize, data->data_sizes, &error);
    HANDLE_CL_ERROR(error);
    bl.data_offsets = clCreateBuffer(context, flags, fmsize, data->data_offsets, &error);
    HANDLE_CL_ERROR(error);

    /* Now calculate device specific sim variables */

    unsigned int max_workgroup_size = WG_FUJ_SZ;

    unsigned int * gridres;
    PS_SET_PTR(data, "gridres", unsigned int, &gridres);

    bl.num_grid_cells = gridres[0]*gridres[1]*gridres[2];

    size_t po2_workgroup_size = 1;
    while (po2_workgroup_size<<1 < max_workgroup_size/* &&
           po2_workgroup_size<<1 < bl.num_grid_cells*/) po2_workgroup_size <<= 1;

    bl.po2_workgroup_size = po2_workgroup_size;

    bl.num_blocks = (bl.num_grid_cells - 1) / (2*po2_workgroup_size) + 1;

    build_program(data, &bl);
    assign_pso_kernel_args(bl);

    bl.block_totals = clCreateBuffer(context, CL_MEM_READ_WRITE, bl.num_blocks*sizeof(unsigned int), NULL, &error);
    HANDLE_CL_ERROR(error);

    bl.backup_prefix_sum = clCreateBuffer(context, CL_MEM_READ_WRITE, bl.num_grid_cells*sizeof(unsigned int), NULL, &error);
    HANDLE_CL_ERROR(error);

    return bl;
}

/**
 * Use specified buffer in computations
 *
 * Sets the arguments of kernels using psdata to this buflist
 *
 * @param bl Buffer list to use
 */
void opencl_use_buflist(psdata_opencl bl) {
    pso = bl;
}

void assign_pso_kernel_args(psdata_opencl bl) {
    set_kernel_args_to_pso(bl, bl.kern_cuboid);
    set_kernel_args_to_pso(bl, bl.kern_find_particle_bins);
    set_kernel_args_to_pso(bl, bl.kern_count_particles_in_bins);
    set_kernel_args_to_pso(bl, bl.kern_prefix_sum);
    set_kernel_args_to_pso(bl, bl.kern_copy_celloffset_to_backup);
    set_kernel_args_to_pso(bl, bl.kern_insert_particles_in_bin_array);

    set_kernel_args_to_pso(bl, bl.kern_compute_density);
    set_kernel_args_to_pso(bl, bl.kern_compute_forces);
    set_kernel_args_to_pso(bl, bl.kern_step_forward);
}

void set_kernel_args_to_pso(psdata_opencl bl, cl_kernel kernel) {
    HANDLE_CL_ERROR(clSetKernelArg(kernel, 0, sizeof(unsigned int), &bl.num_fields));
    HANDLE_CL_ERROR(clSetKernelArg(kernel, 1, sizeof(cl_mem), &bl.names));
    HANDLE_CL_ERROR(clSetKernelArg(kernel, 2, sizeof(cl_mem), &bl.names_offsets));
    HANDLE_CL_ERROR(clSetKernelArg(kernel, 3, sizeof(cl_mem), &bl.dimensions));
    HANDLE_CL_ERROR(clSetKernelArg(kernel, 4, sizeof(cl_mem), &bl.num_dimensions));
    HANDLE_CL_ERROR(clSetKernelArg(kernel, 5, sizeof(cl_mem), &bl.dimensions_offsets));
    HANDLE_CL_ERROR(clSetKernelArg(kernel, 6, sizeof(cl_mem), &bl.entry_sizes));
    HANDLE_CL_ERROR(clSetKernelArg(kernel, 7, sizeof(cl_mem), &bl.data));
    HANDLE_CL_ERROR(clSetKernelArg(kernel, 8, sizeof(cl_mem), &bl.data_sizes));
    HANDLE_CL_ERROR(clSetKernelArg(kernel, 9, sizeof(cl_mem), &bl.data_offsets));
}

/**
 * Release referenced buffer
 */
void free_psdata_opencl(psdata_opencl bl) {
    HANDLE_CL_ERROR(clReleaseMemObject(bl.names));
    HANDLE_CL_ERROR(clReleaseMemObject(bl.names_offsets));
    HANDLE_CL_ERROR(clReleaseMemObject(bl.dimensions));
    HANDLE_CL_ERROR(clReleaseMemObject(bl.num_dimensions));
    HANDLE_CL_ERROR(clReleaseMemObject(bl.dimensions_offsets));
    HANDLE_CL_ERROR(clReleaseMemObject(bl.entry_sizes));
    HANDLE_CL_ERROR(clReleaseMemObject(bl.data));
    HANDLE_CL_ERROR(clReleaseMemObject(bl.data_sizes));
    HANDLE_CL_ERROR(clReleaseMemObject(bl.data_offsets));
    HANDLE_CL_ERROR(clReleaseMemObject(bl.block_totals));

    HANDLE_CL_ERROR(clReleaseKernel(bl.kern_cuboid));
    HANDLE_CL_ERROR(clReleaseKernel(bl.kern_find_particle_bins));
    HANDLE_CL_ERROR(clReleaseKernel(bl.kern_count_particles_in_bins));
    HANDLE_CL_ERROR(clReleaseKernel(bl.kern_prefix_sum));
    HANDLE_CL_ERROR(clReleaseKernel(bl.kern_copy_celloffset_to_backup));
    HANDLE_CL_ERROR(clReleaseKernel(bl.kern_insert_particles_in_bin_array));

    HANDLE_CL_ERROR(clReleaseKernel(bl.kern_compute_density));
    HANDLE_CL_ERROR(clReleaseKernel(bl.kern_compute_forces));
    HANDLE_CL_ERROR(clReleaseKernel(bl.kern_step_forward));

    HANDLE_CL_ERROR(clReleaseProgram(bl.ps_prog));
}

/**
 * Copy buffer data from device to host
 *
 * @param data Host buffer
 * @param bl Device buffer list
 */
void sync_psdata_host_to_device(psdata * data, psdata_opencl bl) {
    unsigned int nf = data->num_fields;

    unsigned int fmsize = nf*sizeof(unsigned int);

    HANDLE_CL_ERROR(clEnqueueWriteBuffer(command_queues[0], bl.names, CL_FALSE, 0,
                                         psdata_names_size(data), (char*) data->names, 0, NULL, NULL));
    HANDLE_CL_ERROR(clEnqueueWriteBuffer(command_queues[0], bl.names_offsets, CL_FALSE, 0,
                                         fmsize, data->names_offsets, 0, NULL, NULL));
    HANDLE_CL_ERROR(clEnqueueWriteBuffer(command_queues[0], bl.dimensions, CL_FALSE, 0, 
                                         psdata_dimensions_size(data), data->dimensions, 0, NULL, NULL));
    HANDLE_CL_ERROR(clEnqueueWriteBuffer(command_queues[0], bl.num_dimensions, CL_FALSE, 0, 
                                         fmsize, data->num_dimensions, 0, NULL, NULL));
    HANDLE_CL_ERROR(clEnqueueWriteBuffer(command_queues[0], bl.dimensions_offsets, CL_FALSE, 0,
                                         fmsize, data->dimensions_offsets, 0, NULL, NULL));
    HANDLE_CL_ERROR(clEnqueueWriteBuffer(command_queues[0], bl.entry_sizes, CL_FALSE, 0,
                                         fmsize, data->entry_sizes, 0, NULL, NULL));
    HANDLE_CL_ERROR(clEnqueueWriteBuffer(command_queues[0], bl.data, CL_FALSE, 0,
                                         psdata_data_size(data), data->data, 0, NULL, NULL));
    HANDLE_CL_ERROR(clEnqueueWriteBuffer(command_queues[0], bl.data_sizes, CL_FALSE, 0,
                                         fmsize, data->data_sizes, 0, NULL, NULL));
    HANDLE_CL_ERROR(clEnqueueWriteBuffer(command_queues[0], bl.data_offsets, CL_FALSE, 0,
                                         fmsize, data->data_offsets, 0, NULL, NULL));

    HANDLE_CL_ERROR(clFinish(command_queues[0]));
}

/**
 * Copy buffer data from host to device
 *
 * @param data Host buffer
 * @param bl Device buffer list
 */
void sync_psdata_device_to_host(psdata * data, psdata_opencl bl) {
    unsigned int nf = data->num_fields;

    unsigned int fmsize = nf*sizeof(unsigned int);

    HANDLE_CL_ERROR(clEnqueueReadBuffer(command_queues[0], bl.names, CL_FALSE, 0,
                                         psdata_names_size(data), data->names, 0, NULL, NULL));
    HANDLE_CL_ERROR(clEnqueueReadBuffer(command_queues[0], bl.names_offsets, CL_FALSE, 0,
                                         fmsize, data->names_offsets, 0, NULL, NULL));
    HANDLE_CL_ERROR(clEnqueueReadBuffer(command_queues[0], bl.dimensions, CL_FALSE, 0, 
                                         psdata_dimensions_size(data), data->dimensions, 0, NULL, NULL));
    HANDLE_CL_ERROR(clEnqueueReadBuffer(command_queues[0], bl.num_dimensions, CL_FALSE, 0, 
                                         fmsize, data->num_dimensions, 0, NULL, NULL));
    HANDLE_CL_ERROR(clEnqueueReadBuffer(command_queues[0], bl.dimensions_offsets, CL_FALSE, 0,
                                         fmsize, data->dimensions_offsets, 0, NULL, NULL));
    HANDLE_CL_ERROR(clEnqueueReadBuffer(command_queues[0], bl.entry_sizes, CL_FALSE, 0,
                                         fmsize, data->entry_sizes, 0, NULL, NULL));
    HANDLE_CL_ERROR(clEnqueueReadBuffer(command_queues[0], bl.data, CL_FALSE, 0,
                                         psdata_data_size(data), data->data, 0, NULL, NULL));
    HANDLE_CL_ERROR(clEnqueueReadBuffer(command_queues[0], bl.data_sizes, CL_FALSE, 0,
                                         fmsize, data->data_sizes, 0, NULL, NULL));
    HANDLE_CL_ERROR(clEnqueueReadBuffer(command_queues[0], bl.data_offsets, CL_FALSE, 0,
                                         fmsize, data->data_offsets, 0, NULL, NULL));

    HANDLE_CL_ERROR(clFinish(command_queues[0]));
}

/**
 * Terminate OpenCL
 *
 * Releases command queues and context. Does not release any buffers,
 * though they may be invalid without the context.
 */
void terminate_ps_opencl() {
    free_opencl_platform_info();

    int i;
    for (i = 0; i < num_command_queues; ++i) {
        HANDLE_CL_ERROR(clReleaseCommandQueue(command_queues[i]));
    }

    free(command_queues);

    HANDLE_CL_ERROR(clReleaseContext(context));

    ready = 0;
}
