#include <stdio.h>
#include <string.h>

#include "build_psdata.h"
#include "config.h"
#include "note.h"
#include "particle_system.h"
#include "opencl/particle_system_host.h"
#include "3rdparty/whereami.h"

#define PI 3.1415926535

void printFieldOffsets(psdata data) {
    for (size_t i = 0; i < data.num_fields; ++i) {
        note(2, "%s: %u\n", data.names + data.names_offsets[i], data.data_offsets[i]);
    }
}

int main(int argc, char *argv[])
{
    set_log_level(1);
    psdata data;

    load_config("/../conf/solid.conf");

    build_psdata_from_string(&data, get_config_section("psdata_specification"));

    double * position;
    double * originalpos;
    double * density0;

    double * rotation;

    PS_GET_FIELD(data, "position", double, &position);
    PS_GET_FIELD(data, "originalpos", double, &originalpos);
    PS_GET_FIELD(data, "density0", double, &density0);

    PS_GET_FIELD(data, "rotation", double, &rotation);

    init_opencl();
        psdata_opencl pso = create_psdata_opencl(&data, get_config_section("opencl_kernel_files"));
            populate_position_cuboid_device_opencl(pso, -1.0, -1.0, -1.0, 1.0, 1.0, 1.0, 6, 6, 6);
            call_for_all_particles_device_opencl(pso, "init_original_position");
            rotate_particles_device_opencl(pso, PI/4, 0, PI/6);

            compute_particle_bins_device_opencl(pso);

            call_for_all_particles_device_opencl(pso, "compute_original_density");
            call_for_all_particles_device_opencl(pso, "compute_density");

            call_for_all_particles_device_opencl(pso, "compute_rotations_and_strains");

            call_for_all_particles_device_opencl(pso, "compute_stresses");

            call_for_all_particles_device_opencl(pso, "compute_forces_solids");

            call_for_all_particles_device_opencl(pso, "step_forward");
        
            sync_psdata_device_to_host(data, pso);
        free_psdata_opencl(&pso);
    terminate_opencl();

    unload_config();

    write_psdata(data, NULL);

    display_psdata(data, NULL);

    free_psdata(&data);

    return 0;
}
