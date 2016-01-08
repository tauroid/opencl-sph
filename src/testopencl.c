#include <stdio.h>

#include "note.h"
#include "opencl/particle_system_host.h"

int main() {
    set_log_level(1);
    init_ps_opencl();
    terminate_ps_opencl();
    return 0;
}
