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

#define WG_FUJ_SZ 384

static int _ready = 0;
static cl_context _context = NULL;
static cl_command_queue _command_queue = NULL;

static Device const * _device = NULL;
static Platform const * _platform = NULL;

static Platform const * _platforms;
static unsigned int _num_platforms;

#ifdef MATLAB_MEX_FILE
static psdata_opencl _pso;
#endif

char * add_field_macros_to_start_of_string(const char * string, psdata * data);

#ifdef MATLAB_MEX_FILE
psdata_opencl * get_stored_psdata_opencl()
{
    return &_pso;
}

void free_stored_psdata_opencl() {
    free_psdata_opencl(&_pso);

    memset(&_pso, 0, sizeof _pso);
}
#endif

/**
 * Initialise OpenCL
 *
 * Detects hardware, creates command queues
 */
void init_opencl()
{
    if (_ready) return;

    get_opencl_platform_info(&_platforms, &_num_platforms);

    ASSERT(_num_platforms > 0);

    // Find the device with the most compute units
    
    char * best_device_name;
    cl_uint most_compute_units = 0;
    
    for (unsigned int p = 0; p < _num_platforms; ++p) {
        for (unsigned int d = 0; d < _platforms[p].num_devices; ++d) {
            if (_platforms[p].devices[d].max_compute_units > most_compute_units) {
                most_compute_units = _platforms[p].devices[d].max_compute_units;
                _platform = &_platforms[p];
                _device = &(_platforms[p].devices[d]);
                best_device_name = _platforms[p].devices[d].name;
            }
        }
    }

    ASSERT(most_compute_units > 0);

    note(1, "Chosen device with most compute units: %s\n", best_device_name);

    const cl_context_properties context_properties[] = {
        CL_CONTEXT_PLATFORM, (cl_context_properties) _platform->id, 0
    };
    
    cl_int error;

    _context = clCreateContextFromType
        ( (const cl_context_properties*) context_properties,
          CL_DEVICE_TYPE_GPU, contextErrorCallback, NULL, &error );

    HANDLE_CL_ERROR(error);

    _command_queue = clCreateCommandQueue
        ( _context, _device->id, CL_QUEUE_PROFILING_ENABLE, &error );

    HANDLE_CL_ERROR(error);

    _ready = 1;
}

static void create_kernels(psdata_opencl * pso)
{
    size_t kernel_names_size;
    const char * kernel_names;

    HANDLE_CL_ERROR(clGetProgramInfo(pso->ps_prog, CL_PROGRAM_NUM_KERNELS, sizeof(size_t), &pso->num_kernels, NULL));
    HANDLE_CL_ERROR(clGetProgramInfo(pso->ps_prog, CL_PROGRAM_KERNEL_NAMES, 0, NULL, &kernel_names_size));

    kernel_names = malloc(kernel_names_size);

    HANDLE_CL_ERROR(clGetProgramInfo(pso->ps_prog, CL_PROGRAM_KERNEL_NAMES, kernel_names_size, (void*) kernel_names, NULL));

    note(1, "%s\n", kernel_names);

    pso->kernel_names = malloc(pso->num_kernels*sizeof(char*));
    pso->kernels = malloc(pso->num_kernels*sizeof(cl_kernel));

    char * kernel_names_ptr = (char*) kernel_names;

    strtok(kernel_names_ptr, ";");

    for (size_t i = 0; i < pso->num_kernels; ++i) {
        char ** kernel_name_ptr = (char**) pso->kernel_names + i;

        *kernel_name_ptr = malloc((strlen(kernel_names_ptr)+1)*sizeof(char));
        strcpy(*kernel_name_ptr, kernel_names_ptr);

        cl_int error;

        *((cl_kernel*) pso->kernels + i) = clCreateKernel(pso->ps_prog, pso->kernel_names[i], &error);
        HANDLE_CL_ERROR(error);

        cl_ulong local_mem, private_mem;
        size_t global_work_size[3], work_group_size;

        clGetKernelWorkGroupInfo(pso->kernels[i], _device->id, CL_KERNEL_LOCAL_MEM_SIZE,
                                 sizeof local_mem, &local_mem, NULL);
        clGetKernelWorkGroupInfo(pso->kernels[i], _device->id, CL_KERNEL_PRIVATE_MEM_SIZE,
                                 sizeof private_mem, &private_mem, NULL);
        clGetKernelWorkGroupInfo(pso->kernels[i], _device->id, CL_KERNEL_GLOBAL_WORK_SIZE,
                                 sizeof global_work_size, global_work_size, NULL);
        clGetKernelWorkGroupInfo(pso->kernels[i], _device->id, CL_KERNEL_WORK_GROUP_SIZE,
                                 sizeof work_group_size, &work_group_size, NULL);

        note(1, "%s:\n"
                "  > Local memory requirements: %lu bytes\n"
                "  > Private memory requirements:  %lu bytes\n"
                "  > Max global work size: %zu, %zu, %zu\n"
                "  > Max workgroup size: %zu\n\n",
             pso->kernel_names[i], local_mem, private_mem,
             global_work_size[0], global_work_size[1], global_work_size[2],
             work_group_size);

        kernel_names_ptr = strtok(NULL, ";");
    }

    note(1, "\n");
}

cl_kernel get_kernel(psdata_opencl pso, const char * name)
{
    for (size_t i = 0; i < pso.num_kernels; ++i) {
        if (strcmp(name, pso.kernel_names[i]) == 0) return pso.kernels[i];
    }

    return NULL;
}

void call_kernel_device_opencl(psdata_opencl pso, const char * kernel_name, cl_uint work_dim,
                               const size_t * global_work_offset, const size_t * global_work_size,
                               const size_t * local_work_size)
{
    cl_kernel kernel = get_kernel(pso, kernel_name);

    ASSERT(kernel != NULL);

    note(1, "Calling kernel %s with work size %u and local size %u\n", kernel_name, *global_work_size, *local_work_size);

    HANDLE_CL_ERROR(clEnqueueNDRangeKernel(_command_queue, kernel, work_dim, global_work_offset,
                                           global_work_size, local_work_size, 0, NULL, NULL));

    HANDLE_CL_ERROR(clFinish(_command_queue));
}

void build_program(psdata * data, psdata_opencl * pso, const char * file_list)
{
    cl_int error;

    /* Get file data */
    {
        /* Just run stuff from the top dir
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
        */

        const char * file_extension = ".cl";
        /*
#ifndef MATLAB_MEX_FILE
        const char * kern_rel_path = "/../kernels/";
#else
        const char * kern_rel_path = "/../../kernels/";
#endif
        */

        const char * kern_rel_path = "kernels/";

        char * file_list_copy = malloc((strlen(file_list)+1)*sizeof(char));

        strcpy(file_list_copy, file_list);

        char * file_name = strtok(file_list_copy, ", \n");

        char * compilation_unit = calloc(1, sizeof(char));

        while (file_name) {
            char * file_path = malloc( ( strlen(kern_rel_path)
                                       + strlen(file_name)
                                       + strlen(file_extension)
                                       + 1
                                       ) * sizeof(char));
            
            sprintf(file_path, "%s%s%s", kern_rel_path, file_name, file_extension);

            note(1, "Adding OpenCL file at %s\n", file_path);

            long int file_length;

            FILE * f = fopen(file_path, "rb");

            free(file_path);

            ASSERT(f != NULL);

            fseek(f, 0, SEEK_END);
            file_length = ftell(f);
            fseek(f, 0, SEEK_SET);

            size_t compilation_unit_len = strlen(compilation_unit);

            size_t new_length = compilation_unit_len + file_length / sizeof(char);

            char * compilation_unit_temp = malloc((new_length + 1) * sizeof(char));

            strcpy(compilation_unit_temp, compilation_unit);
            fread(compilation_unit_temp + compilation_unit_len, sizeof(char), file_length, f);

            fclose(f);

            compilation_unit_temp[new_length] = '\0';

            free(compilation_unit);

            compilation_unit = compilation_unit_temp;

            file_name = strtok(NULL, ", \n");
        }

        free(file_list_copy);

        /*
#ifndef MATLAB_MEX_FILE
        free(exe_path);
#endif
        */

        char * compilation_unit_with_macros = add_field_macros_to_start_of_string(compilation_unit, data);
        free(compilation_unit);

        size_t cu_macros_length = strlen(compilation_unit_with_macros);

        pso->ps_prog = clCreateProgramWithSource(_context, 1, (const char **) &compilation_unit_with_macros, &cu_macros_length, &error);
        HANDLE_CL_ERROR(error);

        free(compilation_unit_with_macros);
    }

    cl_int build_error = clBuildProgram(pso->ps_prog, 1, &_device->id, "-cl-fast-relaxed-math", NULL, NULL);

    if (build_error != CL_SUCCESS) {
        char * error_log;
        size_t log_length;

        HANDLE_CL_ERROR(clGetProgramBuildInfo(pso->ps_prog, _device->id,
                                              CL_PROGRAM_BUILD_LOG, 0, NULL, &log_length));

        error_log = malloc(log_length*sizeof(char));

        HANDLE_CL_ERROR(clGetProgramBuildInfo(pso->ps_prog, _device->id,
                                              CL_PROGRAM_BUILD_LOG, log_length, error_log, NULL));

        note(2, "%s\n", error_log);

        free(error_log);

        HANDLE_CL_ERROR(build_error);
    }
}

/* Allocates new string */
char * add_field_macros_to_start_of_string(const char * string, psdata * data)
{
    const char * start = "#define ";
    const char * middle = " (((global char *) data) + data_offsets[";
    const char * end = "])\n";

    size_t start_size = strlen(start);
    size_t middle_size = strlen(middle);
    size_t end_size = strlen(end);

    size_t string_size = 0;

    unsigned int f = 0;
    for (; f < data->num_fields; ++f) {
        string_size += start_size + strlen(data->names + data->names_offsets[f]) + 3 + (int)(f == 0 ? 1 : floor(log10(f)) + 1) + 1;
        string_size += start_size + strlen(data->names + data->names_offsets[f]) + 2 + middle_size + (int)(f == 0 ? 1 : floor(log10(f)) + 1) + end_size;
    }

    char * newstring = malloc(strlen(string) + string_size + 1);

    char * newstring_ptr = newstring;

    for (f = 0; f < data->num_fields; ++f) {
        size_t field_numeral_length = (f == 0 ? 1 : floor(log10(f)) + 1);

        // Field number
        strcpy(newstring_ptr, start); newstring_ptr += start_size;
        strcpy(newstring_ptr, data->names + data->names_offsets[f]); newstring_ptr += strlen(data->names + data->names_offsets[f]);
        strcpy(newstring_ptr, "_f "); newstring_ptr += 3;
        sprintf(newstring_ptr, "%d\n", f); newstring_ptr += field_numeral_length + 1;

        // Field macro
        strcpy(newstring_ptr, start); newstring_ptr += start_size;
        strcpy(newstring_ptr, data->names + data->names_offsets[f]); newstring_ptr += strlen(data->names + data->names_offsets[f]);
        strcpy(newstring_ptr, "_m"); newstring_ptr += 2;
        strcpy(newstring_ptr, middle); newstring_ptr += middle_size;
        sprintf(newstring_ptr, "%d", f); newstring_ptr += field_numeral_length;
        strcpy(newstring_ptr, end); newstring_ptr += end_size;
    }

    strcpy(newstring_ptr, string);

    return newstring;
}

static void zero_gridcount_device_opencl(psdata_opencl pso)
{
    unsigned int * gridres;

    PS_GET_FIELD(pso.host_psdata, "gridres", unsigned int, &gridres);

    size_t num_workgroups = gridres[0]*gridres[1]*gridres[2] / pso.po2_workgroup_size + 1;
    size_t num_workitems = num_workgroups * pso.po2_workgroup_size;

    call_kernel_device_opencl(pso, "zero_gridcount", 1, 0, &num_workitems, &pso.po2_workgroup_size);
}

static void bin_and_count_device_opencl(psdata_opencl pso)
{
    unsigned int * pnum_ptr;

    PS_GET_FIELD(pso.host_psdata, "pnum", unsigned int, &pnum_ptr)
    unsigned int pnum = *pnum_ptr;

    size_t num_p_workgroups = pnum / pso.po2_workgroup_size + 1;
    size_t num_p_workitems = num_p_workgroups * pso.po2_workgroup_size;

    cl_kernel find_particle_bins = get_kernel(pso, "find_particle_bins");
    cl_kernel count_particles_in_bins = get_kernel(pso, "count_particles_in_bins");

    ASSERT(find_particle_bins != NULL && count_particles_in_bins != NULL);

    HANDLE_CL_ERROR(clEnqueueNDRangeKernel(_command_queue, find_particle_bins, 1,
                                           NULL, &num_p_workitems, &pso.po2_workgroup_size, 0, NULL, NULL));

    HANDLE_CL_ERROR(clFinish(_command_queue));

    zero_gridcount_device_opencl(pso);

    HANDLE_CL_ERROR(clEnqueueNDRangeKernel(_command_queue, count_particles_in_bins, 1,
                                           NULL, &num_p_workitems, &pso.po2_workgroup_size, 0, NULL, NULL));

    HANDLE_CL_ERROR(clFinish(_command_queue));
}

static void prefix_sum_device_opencl(psdata_opencl pso)
{
    cl_kernel prefix_sum = get_kernel(pso, "prefix_sum");

    ASSERT(prefix_sum != NULL);

    size_t num_work_groups = pso.num_blocks;
    size_t num_work_items = num_work_groups * pso.po2_workgroup_size;

    HANDLE_CL_ERROR(clEnqueueNDRangeKernel(_command_queue, prefix_sum, 1, NULL,
                                               &num_work_items, &pso.po2_workgroup_size, 0, NULL, NULL));

    HANDLE_CL_ERROR(clFinish(_command_queue));
}

static void copy_celloffset_to_backup_device_opencl(psdata_opencl pso)
{
    cl_kernel copy_celloffset_to_backup = get_kernel(pso, "copy_celloffset_to_backup");

    ASSERT(copy_celloffset_to_backup != NULL);

    size_t num_work_items = pso.num_blocks * 2 * pso.po2_workgroup_size;

    HANDLE_CL_ERROR(clEnqueueNDRangeKernel(_command_queue, copy_celloffset_to_backup, 1, NULL, &num_work_items, &pso.po2_workgroup_size, 0, NULL, NULL));

    HANDLE_CL_ERROR(clFinish(_command_queue));
}

static void insert_particles_in_bin_array_device_opencl(psdata_opencl pso)
{
    int * pnum_ptr;

    PS_GET_FIELD(pso.host_psdata, "pnum", int, &pnum_ptr);
    int n = pnum_ptr[1];

    size_t num_workgroups = n / pso.po2_workgroup_size + 1;
    size_t num_work_items = num_workgroups * pso.po2_workgroup_size;

    cl_kernel insert_particles_in_bin_array = get_kernel(pso, "insert_particles_in_bin_array");

    ASSERT(insert_particles_in_bin_array != NULL);

    HANDLE_CL_ERROR(clEnqueueNDRangeKernel(_command_queue, insert_particles_in_bin_array, 1, NULL, &num_work_items, &pso.po2_workgroup_size, 0, NULL, NULL));
    
    HANDLE_CL_ERROR(clFinish(_command_queue));
}

void compute_particle_bins_device_opencl(psdata_opencl pso)
{
    bin_and_count_device_opencl(pso);
    prefix_sum_device_opencl(pso);
    copy_celloffset_to_backup_device_opencl(pso);
    insert_particles_in_bin_array_device_opencl(pso);
}

void compute_density_device_opencl(psdata_opencl pso)
{
    unsigned int * n_ptr;

    PS_GET_FIELD(pso.host_psdata, "n", unsigned int, &n_ptr);
    unsigned int n = *n_ptr;

    size_t num_workgroups = n / pso.po2_workgroup_size + 1;
    size_t num_workitems = num_workgroups * pso.po2_workgroup_size;

    cl_kernel compute_density = get_kernel(pso, "compute_density");

    ASSERT(compute_density != NULL);

    note(1, "Starting density comp with %u workitems, workgroup size %d\n", num_workitems, pso.po2_workgroup_size);
    HANDLE_CL_ERROR(clEnqueueNDRangeKernel(_command_queue, compute_density, 1, NULL,
                                           &num_workitems, &pso.po2_workgroup_size, 0, NULL, NULL));

    HANDLE_CL_ERROR(clFinish(_command_queue));
}

void compute_forces_device_opencl(psdata_opencl pso)
{
    unsigned int * n_ptr;

    PS_GET_FIELD(pso.host_psdata, "n", unsigned int, &n_ptr);
    unsigned int n = *n_ptr;

    size_t num_workgroups = n / pso.po2_workgroup_size + 1;
    size_t num_workitems = num_workgroups * pso.po2_workgroup_size;

    cl_kernel compute_forces = get_kernel(pso, "compute_forces");

    ASSERT(compute_forces != NULL);
    
    HANDLE_CL_ERROR(clEnqueueNDRangeKernel(_command_queue, compute_forces, 1, NULL,
                                           &num_workitems, &pso.po2_workgroup_size, 0, NULL, NULL));

    HANDLE_CL_ERROR(clFinish(_command_queue));
}

void step_forward_device_opencl(psdata_opencl pso)
{
    unsigned int * n_ptr;

    PS_GET_FIELD(pso.host_psdata, "n", unsigned int, &n_ptr);
    unsigned int n = *n_ptr;

    size_t num_workgroups = n / pso.po2_workgroup_size + 1;
    size_t num_workitems = num_workgroups * pso.po2_workgroup_size;

    cl_kernel step_forward = get_kernel(pso, "step_forward");

    ASSERT(step_forward != NULL);
    
    HANDLE_CL_ERROR(clEnqueueNDRangeKernel(_command_queue, step_forward, 1, NULL,
                                           &num_workitems, &pso.po2_workgroup_size, 0, NULL, NULL));

    HANDLE_CL_ERROR(clFinish(_command_queue));
}

void call_for_all_particles_device_opencl(psdata_opencl pso, const char * kernel_name)
{
    unsigned int * pN;

    PS_GET_FIELD(pso.host_psdata, "n", unsigned int, &pN);
    unsigned int n = *pN;

    size_t num_workgroups = (n - 1) / pso.po2_workgroup_size + 1;
    size_t num_workitems = num_workgroups * pso.po2_workgroup_size;

    call_kernel_device_opencl(pso, kernel_name, 1, 0, &num_workitems, &pso.po2_workgroup_size);
}

void populate_position_cuboid_device_opencl(psdata_opencl pso,
                                            float x1, float y1, float z1,
                                            float x2, float y2, float z2,
                                            unsigned int xsize,
                                            unsigned int ysize,
                                            unsigned int zsize)
{
    size_t work_group_edge = (size_t) pow
        ((float) _device->max_workgroup_size, 1.0/3.0);
    size_t local_work_size[] = { work_group_edge, work_group_edge, work_group_edge };
    size_t global_work_size[] = { (xsize/work_group_edge + 1) * work_group_edge,
                                  (ysize/work_group_edge + 1) * work_group_edge,
                                  (zsize/work_group_edge + 1) * work_group_edge };

    cl_float3 corner1 = {{ x1, y1, z1 }};
    cl_float3 corner2 = {{ x2, y2, z2 }};
    cl_uint3 size = {{ xsize, ysize, zsize }};

    cl_kernel cuboid = get_kernel(pso, "populate_position_cuboid");

    ASSERT(cuboid != NULL);

    HANDLE_CL_ERROR(clSetKernelArg(cuboid, NUM_PS_ARGS, sizeof(cl_float3), &corner1));
    HANDLE_CL_ERROR(clSetKernelArg(cuboid, NUM_PS_ARGS+1, sizeof(cl_float3), &corner2));
    HANDLE_CL_ERROR(clSetKernelArg(cuboid, NUM_PS_ARGS+2, sizeof(cl_uint3), &size));

    HANDLE_CL_ERROR(clEnqueueNDRangeKernel(_command_queue, cuboid, 3, NULL,
                                           global_work_size, local_work_size,
                                           0, NULL, NULL));
    HANDLE_CL_ERROR(clFinish(_command_queue));
}

void set_body_position_device_opencl(psdata_opencl pso, unsigned int body, float * position)
{
    cl_float3 pos = {{position[0], position[1], position[2]}};

    cl_kernel set_body_position = get_kernel(pso, "set_body_position");

    ASSERT(set_body_position != NULL);

    HANDLE_CL_ERROR(clSetKernelArg(set_body_position, NUM_PS_ARGS, sizeof(cl_uint), &body));
    HANDLE_CL_ERROR(clSetKernelArg(set_body_position, NUM_PS_ARGS+1, sizeof(cl_float3), &pos));

    call_for_all_particles_device_opencl(pso, "set_body_position");
}

void set_body_rotation_device_opencl(psdata_opencl pso, unsigned int body, float * euler_angles)
{
    cl_float3 euler = {{euler_angles[0], euler_angles[1], euler_angles[2]}};

    cl_kernel set_body_position = get_kernel(pso, "set_body_rotation");

    ASSERT(set_body_position != NULL);

    HANDLE_CL_ERROR(clSetKernelArg(set_body_position, NUM_PS_ARGS, sizeof(cl_uint), &body));
    HANDLE_CL_ERROR(clSetKernelArg(set_body_position, NUM_PS_ARGS+1, sizeof(cl_float3), &euler));

    call_for_all_particles_device_opencl(pso, "set_body_rotation");
}

void rotate_particles_device_opencl(psdata_opencl pso, float angle_x, float angle_y, float angle_z)
{
    unsigned int * pN;

    PS_GET_FIELD(pso.host_psdata, "n", unsigned int, &pN);

    unsigned int n = *pN;

    size_t num_workgroups = n / pso.po2_workgroup_size + 1;
    size_t num_workitems = num_workgroups * pso.po2_workgroup_size;

    cl_kernel rotate_particles = get_kernel(pso, "rotate_particles");

    ASSERT(rotate_particles != NULL);

    HANDLE_CL_ERROR(clSetKernelArg(rotate_particles, NUM_PS_ARGS, sizeof(cl_float), &angle_x));
    HANDLE_CL_ERROR(clSetKernelArg(rotate_particles, NUM_PS_ARGS+1, sizeof(cl_float), &angle_y));
    HANDLE_CL_ERROR(clSetKernelArg(rotate_particles, NUM_PS_ARGS+2, sizeof(cl_float), &angle_z));

    HANDLE_CL_ERROR(clEnqueueNDRangeKernel(_command_queue, rotate_particles, 1, NULL,
                                           &num_workitems, &pso.po2_workgroup_size,
                                           0, NULL, NULL));

    HANDLE_CL_ERROR(clFinish(_command_queue));
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
psdata_opencl create_psdata_opencl(psdata * data, const char * file_list)
{
    psdata_opencl pso;

    pso.host_psdata = *data;

    cl_mem_flags flags = CL_MEM_COPY_HOST_PTR;

    cl_int error;

    unsigned int nf = data->num_fields;

    unsigned int fmsize = nf*sizeof(unsigned int);

    pso.num_fields = nf;
    pso.names = clCreateBuffer(_context, flags, psdata_names_size(*data), (char*) data->names, &error);
    HANDLE_CL_ERROR(error);
    pso.names_offsets = clCreateBuffer(_context, flags, fmsize, data->names_offsets, &error);
    HANDLE_CL_ERROR(error);
    pso.dimensions = clCreateBuffer(_context, flags, psdata_dimensions_size(*data), data->dimensions, &error);
    HANDLE_CL_ERROR(error);
    pso.num_dimensions = clCreateBuffer(_context, flags, fmsize, data->num_dimensions, &error);
    HANDLE_CL_ERROR(error);
    pso.dimensions_offsets = clCreateBuffer(_context, flags, fmsize, data->dimensions_offsets, &error);
    HANDLE_CL_ERROR(error);
    pso.entry_sizes = clCreateBuffer(_context, flags, fmsize, data->entry_sizes, &error);
    HANDLE_CL_ERROR(error);
    pso.data = clCreateBuffer(_context, flags, psdata_data_size(*data), data->data, &error);
    HANDLE_CL_ERROR(error);
    pso.data_sizes = clCreateBuffer(_context, flags, fmsize, data->data_sizes, &error);
    HANDLE_CL_ERROR(error);
    pso.data_offsets = clCreateBuffer(_context, flags, fmsize, data->data_offsets, &error);
    HANDLE_CL_ERROR(error);

    /* Now calculate device specific sim variables */

    unsigned int max_workgroup_size = WG_FUJ_SZ*2; //_device->max_workgroup_size;

    unsigned int * gridres;
    PS_GET_FIELD(*data, "gridres", unsigned int, &gridres);

    pso.num_grid_cells = gridres[0]*gridres[1]*gridres[2];

    size_t po2_workgroup_size = 1;
    while (po2_workgroup_size<<1 <= max_workgroup_size/2/* &&
           po2_workgroup_size<<1 < pso.num_grid_cells*/) po2_workgroup_size <<= 1;

    pso.po2_workgroup_size = po2_workgroup_size;

    pso.num_blocks = (pso.num_grid_cells - 1) / (2*po2_workgroup_size) + 1;

    build_program(data, &pso, file_list);
    create_kernels(&pso);

    pso.block_totals = clCreateBuffer(_context, CL_MEM_READ_WRITE, pso.num_blocks*sizeof(unsigned int), NULL, &error);
    HANDLE_CL_ERROR(error);

    pso.backup_prefix_sum = clCreateBuffer(_context, CL_MEM_READ_WRITE, pso.num_grid_cells*sizeof(unsigned int), NULL, &error);
    HANDLE_CL_ERROR(error);

    assign_pso_kernel_args(pso);

    return pso;
}

#ifdef MATLAB_MEX_FILE
/**
 * Feed specified buffer to kernel PSO_ARGS
 *
 * @param pso Buffer list to use
 */
void opencl_use_buflist(psdata_opencl pso)
{
    _pso = pso;
}
#endif

void assign_pso_kernel_args(psdata_opencl pso)
{
    for (size_t i = 0; i < pso.num_kernels; ++i) {
        set_kernel_args_to_pso(pso, pso.kernels[i]);
    }

    cl_kernel prefix_sum = get_kernel(pso, "prefix_sum");

    ASSERT(prefix_sum != NULL);

    HANDLE_CL_ERROR(clSetKernelArg(prefix_sum, NUM_PS_ARGS, 2*pso.po2_workgroup_size*sizeof(unsigned int), NULL));
    HANDLE_CL_ERROR(clSetKernelArg(prefix_sum, NUM_PS_ARGS + 1, sizeof(unsigned int), &pso.num_grid_cells));
    HANDLE_CL_ERROR(clSetKernelArg(prefix_sum, NUM_PS_ARGS + 2, sizeof(cl_mem), &pso.block_totals));
    HANDLE_CL_ERROR(clSetKernelArg(prefix_sum, NUM_PS_ARGS + 3, sizeof(unsigned int), &pso.num_blocks));

    cl_kernel copy_celloffset_to_backup = get_kernel(pso, "copy_celloffset_to_backup");

    ASSERT(copy_celloffset_to_backup != NULL);

    HANDLE_CL_ERROR(clSetKernelArg(copy_celloffset_to_backup, NUM_PS_ARGS, sizeof(cl_mem), &pso.backup_prefix_sum));
    HANDLE_CL_ERROR(clSetKernelArg(copy_celloffset_to_backup, NUM_PS_ARGS+1, sizeof(unsigned int), &pso.num_grid_cells));

    cl_kernel insert_particles_in_bin_array = get_kernel(pso, "insert_particles_in_bin_array");

    ASSERT(insert_particles_in_bin_array != NULL);

    HANDLE_CL_ERROR(clSetKernelArg(insert_particles_in_bin_array, NUM_PS_ARGS, sizeof(cl_mem), &pso.backup_prefix_sum));
}

void set_kernel_args_to_pso(psdata_opencl pso, cl_kernel kernel)
{
    HANDLE_CL_ERROR(clSetKernelArg(kernel, 0, sizeof(unsigned int), &pso.num_fields));
    HANDLE_CL_ERROR(clSetKernelArg(kernel, 1, sizeof(cl_mem), &pso.names));
    HANDLE_CL_ERROR(clSetKernelArg(kernel, 2, sizeof(cl_mem), &pso.names_offsets));
    HANDLE_CL_ERROR(clSetKernelArg(kernel, 3, sizeof(cl_mem), &pso.dimensions));
    HANDLE_CL_ERROR(clSetKernelArg(kernel, 4, sizeof(cl_mem), &pso.num_dimensions));
    HANDLE_CL_ERROR(clSetKernelArg(kernel, 5, sizeof(cl_mem), &pso.dimensions_offsets));
    HANDLE_CL_ERROR(clSetKernelArg(kernel, 6, sizeof(cl_mem), &pso.entry_sizes));
    HANDLE_CL_ERROR(clSetKernelArg(kernel, 7, sizeof(cl_mem), &pso.data));
    HANDLE_CL_ERROR(clSetKernelArg(kernel, 8, sizeof(cl_mem), &pso.data_sizes));
    HANDLE_CL_ERROR(clSetKernelArg(kernel, 9, sizeof(cl_mem), &pso.data_offsets));
}

/**
 * Release referenced buffer
 */
void free_psdata_opencl(psdata_opencl * pso)
{
    HANDLE_CL_ERROR(clReleaseMemObject(pso->names));
    HANDLE_CL_ERROR(clReleaseMemObject(pso->names_offsets));
    HANDLE_CL_ERROR(clReleaseMemObject(pso->dimensions));
    HANDLE_CL_ERROR(clReleaseMemObject(pso->num_dimensions));
    HANDLE_CL_ERROR(clReleaseMemObject(pso->dimensions_offsets));
    HANDLE_CL_ERROR(clReleaseMemObject(pso->entry_sizes));
    HANDLE_CL_ERROR(clReleaseMemObject(pso->data));
    HANDLE_CL_ERROR(clReleaseMemObject(pso->data_sizes));
    HANDLE_CL_ERROR(clReleaseMemObject(pso->data_offsets));
    HANDLE_CL_ERROR(clReleaseMemObject(pso->block_totals));

    for (size_t i = 0; i < pso->num_kernels; ++i) {
        HANDLE_CL_ERROR(clReleaseKernel(pso->kernels[i]));
        free(*((char**) pso->kernel_names + i));
    }

    free((void*)pso->kernels);
    free((void*)pso->kernel_names);

    pso->num_kernels = 0;

    HANDLE_CL_ERROR(clReleaseProgram(pso->ps_prog));
}

/**
 * Copy buffer data from device to host
 *
 * @param data Host buffer
 * @param pso Device buffer list
 */
void sync_psdata_host_to_device(psdata data, psdata_opencl pso, int full)
{
    unsigned int nf = data.num_fields;

    unsigned int fmsize = nf*sizeof(unsigned int);

    HANDLE_CL_ERROR(clEnqueueWriteBuffer(_command_queue, pso.data, CL_FALSE, 0,
                                         psdata_data_size(data), data.data, 0, NULL, NULL));

    if (full) {
        HANDLE_CL_ERROR(clEnqueueWriteBuffer(_command_queue, pso.names, CL_FALSE, 0,
                                             psdata_names_size(data), (char*) data.names, 0, NULL, NULL));
        HANDLE_CL_ERROR(clEnqueueWriteBuffer(_command_queue, pso.names_offsets, CL_FALSE, 0,
                                             fmsize, data.names_offsets, 0, NULL, NULL));
        HANDLE_CL_ERROR(clEnqueueWriteBuffer(_command_queue, pso.dimensions, CL_FALSE, 0, 
                                             psdata_dimensions_size(data), data.dimensions, 0, NULL, NULL));
        HANDLE_CL_ERROR(clEnqueueWriteBuffer(_command_queue, pso.num_dimensions, CL_FALSE, 0, 
                                             fmsize, data.num_dimensions, 0, NULL, NULL));
        HANDLE_CL_ERROR(clEnqueueWriteBuffer(_command_queue, pso.dimensions_offsets, CL_FALSE, 0,
                                             fmsize, data.dimensions_offsets, 0, NULL, NULL));
        HANDLE_CL_ERROR(clEnqueueWriteBuffer(_command_queue, pso.entry_sizes, CL_FALSE, 0,
                                             fmsize, data.entry_sizes, 0, NULL, NULL));
        HANDLE_CL_ERROR(clEnqueueWriteBuffer(_command_queue, pso.data_sizes, CL_FALSE, 0,
                                             fmsize, data.data_sizes, 0, NULL, NULL));
        HANDLE_CL_ERROR(clEnqueueWriteBuffer(_command_queue, pso.data_offsets, CL_FALSE, 0,
                                             fmsize, data.data_offsets, 0, NULL, NULL));
    }

    HANDLE_CL_ERROR(clFinish(_command_queue));
}

// Don't write to the same field twice or there will be a data race and also why would you do that
void sync_psdata_fields_host_to_device(psdata data, psdata_opencl pso, size_t num_fields, const char * const * const field_names)
{
    for (size_t i = 0; i < num_fields; ++i) {
        if (field_names[i] == NULL) continue;

        int f = get_field_psdata(data, field_names[i]);

        if (f == -1) {
            note(1, "Field %s not found.\n", field_names[i]);
            continue;
        }

        HANDLE_CL_ERROR(clEnqueueWriteBuffer(_command_queue, pso.data, CL_FALSE, data.data_offsets[f],
                                             data.data_sizes[f], (char*) data.data + data.data_offsets[f], 0, NULL, NULL));
    }

    HANDLE_CL_ERROR(clFinish(_command_queue));
}

/**
 * Copy buffer data from host to device
 *
 * @param data Host buffer
 * @param pso Device buffer list
 */
void sync_psdata_device_to_host(psdata data, psdata_opencl pso)
{
    HANDLE_CL_ERROR(clEnqueueReadBuffer(_command_queue, pso.data, CL_TRUE, 0,
                                         psdata_data_size(data), data.data, 0, NULL, NULL));
}

void sync_psdata_fields_device_to_host(psdata data, psdata_opencl pso, size_t num_fields, const char * const * const field_names)
{
    for (size_t i = 0; i < num_fields; ++i) {
        int f = get_field_psdata(data, field_names[i]);

        if (f == -1) {
            note(1, "Field %s not found.\n", field_names[i]);
            continue;
        }

        HANDLE_CL_ERROR(clEnqueueReadBuffer(_command_queue, pso.data, CL_FALSE, data.data_offsets[f],
                                            data.data_sizes[f], (char*) data.data + data.data_offsets[f], 0, NULL, NULL));
    }
    
    HANDLE_CL_ERROR(clFinish(_command_queue));
}

/**
 * Terminate OpenCL
 *
 * Releases command queues and context. Does not release any buffers,
 * though they may be invalid without the context.
 */
void terminate_opencl()
{
    free_opencl_platform_info();

    _platform = NULL;
    _device = NULL;

    HANDLE_CL_ERROR(clReleaseCommandQueue(_command_queue));

    _command_queue = NULL;

    HANDLE_CL_ERROR(clReleaseContext(_context));

    _context = NULL;

    _ready = 0;
}
