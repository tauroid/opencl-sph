#include <CL/opencl.h>
#include "../particle_system.h"

typedef struct {
    unsigned int num_fields;
    cl_mem names;
    cl_mem names_offsets;
    cl_mem dimensions;
    cl_mem num_dimensions;
    cl_mem dimensions_offsets;
    cl_mem entry_sizes;

    cl_mem data;
    cl_mem data_sizes;
    cl_mem data_offsets;

    /* Device only buffers
     *
     * The gridcount stuff should probably go here too but I'll leave
     * it for now because lazy
     */

    cl_mem block_totals;
    cl_mem backup_prefix_sum;

    /* Program stuff, it's per psdata as the program changes based on data array layout */

    cl_kernel kern_cuboid;
    cl_kernel kern_find_particle_bins;
    cl_kernel kern_count_particles_in_bins;
    cl_kernel kern_prefix_sum;
    cl_kernel kern_copy_celloffset_to_backup;
    cl_kernel kern_insert_particles_in_bin_array;

    cl_kernel kern_compute_density;
    cl_kernel kern_compute_forces;
    cl_kernel kern_step_forward;

    cl_program ps_prog;

    /* System dependent variables, also computed on buffer creation
     *
     * These are metadata for the various operations the device will perform
     */
    unsigned int num_grid_cells;
    size_t po2_workgroup_size;
    unsigned int num_blocks;
} psdata_opencl;

#ifdef MATLAB_MEX_FILE
psdata_opencl get_stored_psdata_opencl();
#endif

void init_ps_opencl();
psdata_opencl create_psdata_opencl(psdata * data);
void opencl_use_buflist(psdata_opencl bl);
void assign_pso_kernel_args(psdata_opencl bl);
void set_kernel_args_to_pso(psdata_opencl bl, cl_kernel kernel);
void free_psdata_opencl(psdata_opencl bl);
void terminate_ps_opencl();

void sync_psdata_device_to_host(psdata * data, psdata_opencl bl);
void sync_psdata_host_to_device(psdata * data, psdata_opencl bl);

void populate_position_cuboid_device_opencl(double x1, double y1, double z1,
                                            double x2, double y2, double z2,
                                            unsigned int xsize,
                                            unsigned int ysize,
                                            unsigned int zsize);

void bin_and_count_device_opencl(psdata * data);
void prefix_sum_device_opencl(psdata * data);
void copy_celloffset_to_backup_device_opencl(psdata * data);
void insert_particles_in_bin_array_device_opencl(psdata * data);
void find_particle_bins_device_opencl(psdata * data);

void compute_density_device_opencl(psdata * data);
void compute_forces_device_opencl(psdata * data);
void step_forward_device_opencl(psdata * data);
