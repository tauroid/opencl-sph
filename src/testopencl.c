#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "build_psdata.h"
#include "note.h"
#include "particle_system.h"
#include "opencl/particle_system_host.h"
#include "3rdparty/whereami.h"

int main() {
    set_log_level(1);
    psdata data;
    psdata * d = &data;

    int exe_path_len = wai_getExecutablePath(NULL, 0, NULL);
    char * exe_path = malloc((exe_path_len+1)*sizeof(char));
    wai_getExecutablePath(exe_path, exe_path_len, NULL);

    exe_path[exe_path_len] = 0x0;
    char * lastslash = strrchr(exe_path, '/');

    if (lastslash != NULL) {
        exe_path_len = (int)( (lastslash - exe_path) / sizeof(char) );
        exe_path[exe_path_len] = 0x0;
    }

    const char * kern_rel_path = "/../conf/fluid.conf";

    char * kern_path = malloc((strlen(exe_path)+strlen(kern_rel_path)+1)*sizeof(char));
    sprintf(kern_path, "%s%s", exe_path, kern_rel_path);

    build_psdata(d, kern_path);

    free(exe_path); free(kern_path);

    unsigned int * gridcell;
    unsigned int * gridcount;
    unsigned int * celloffset;
    unsigned int * cellparticles;
    double * position;
    double * density;

    PS_SET_PTR(d, "gridcell", unsigned int, &gridcell);
    PS_SET_PTR(d, "gridcount", unsigned int, &gridcount);
    PS_SET_PTR(d, "celloffset", unsigned int, &celloffset);
    PS_SET_PTR(d, "cellparticles", unsigned int, &cellparticles);
    PS_SET_PTR(d, "force", double, &position);
    PS_SET_PTR(d, "density", double, &density);

    init_ps_opencl();
        psdata_opencl bl = create_psdata_opencl(d);
            opencl_use_buflist(bl);

            populate_position_cuboid_device_opencl(0.5, 0.5, 0.5, 2.5, 2.5, 2.5, 3, 3, 3);

            sync_psdata_device_to_host(d, bl);

            // The 1 at the end of celloffset is nothing to do with prefix sum
            bin_and_count_device_opencl(d);
            prefix_sum_device_opencl(d);
            copy_celloffset_to_backup_device_opencl(d);
            insert_particles_in_bin_array_device_opencl(d);

            compute_density_device_opencl(d);
            compute_forces_device_opencl(d);

            step_forward_device_opencl(d);
        
            sync_psdata_device_to_host(d, bl);
        free_psdata_opencl(bl);
    terminate_ps_opencl();

    int i;

    note(2, "Num grid cells: %u\n", bl.num_grid_cells);

    for (i = 0; i < 64; ++i) {
        note(2, "%.4g, %.4g, %.4g\n", position[i*3], position[i*3 + 1], position[i*3 + 2]);
    }

    note(2, "Particle cells\n");
    for (i = 0; i < 64; ++i) {
        note(2, "%d, ", gridcell[i]);
    }
    note(2, "\n");

    note(2, "Grid cell counts\n");
    for (i = 0; i < 64; ++i) {
        note(2, "%d, ", gridcount[i]);
    }
    note(2, "\n");

    note(2, "Grid cell prefix sum\n");
    for (i = 0; i < 64; ++i) {
        note(2, "%d, ", celloffset[i]);
    }
    note(2, "\n");

    note(2, "Cell particles\n");
    for (i = 0; i < 64; ++i) {
        note(2, "%d, ", cellparticles[i]);
    }
    note(2, "\n");
    
    note(2, "Density\n");
    for (i = 0; i < 64; ++i) {
        note(2, "%g, ", density[i]);
    }
    note(2, "\n");
    
    return 0;
}
