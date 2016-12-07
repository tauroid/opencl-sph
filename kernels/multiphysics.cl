kernel void compute_forces_multiphysics (PSO_ARGS) {
    USE_GRID_PROPS

    USE_FIELD_FIRST_VALUE(n, uint) USE_FIELD_FIRST_VALUE(mass, double)
    USE_FIELD_FIRST_VALUE(restdens, double) USE_FIELD_FIRST_VALUE(stiffness, double)

    USE_FIELD(originalpos, double) USE_FIELD_FIRST_VALUE(smoothingradius, double)
    USE_FIELD(stress, double) USE_FIELD_FIRST_VALUE(mass, double) USE_FIELD(density0, double)
    USE_FIELD(rotation, double) USE_FIELD(force, double) USE_FIELD(velocity, double)
    USE_FIELD_FIRST_VALUE(viscosity, double) USE_FIELD(density, double) USE_FIELD(position, double)

    USE_FIELD(particletype, uint)

    unsigned int i = get_global_id(0);

    if (i >= n) return;

    double3 ipos = vload3(i, position);
    double3 ivel = vload3(i, velocity);

    INIT_FLUIDS_FORCE_COMPUTATION
    INIT_SOLIDS_FORCE_COMPUTATION

    double3 f_ext = (double3)(0, 0, 0);

    FOR_PARTICLES_IN_RANGE(i, j,
        if (j == i) continue;

        if (particletype[i] == 1 && particletype[j] == 1) {
            SOLIDS_FORCE_COMPUTATION
        } else if (particletype[i] == 2 && particletype[j] == 2) {
            FLUIDS_FORCE_COMPUTATION
        } else {
            // f_ext += Something something pressure gradient
        }
    )

    FINALISE_SOLIDS_FORCE_COMPUTATION
    FINALISE_FLUIDS_FORCE_COMPUTATION

    double3 f = f_i + f_e + f_v + f_ext;

    APPLY_CUBE_BOUNDS(ipos, f, -2.0, 2.0)

    vstore3(f, i, force);
}
