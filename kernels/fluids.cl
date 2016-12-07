kernel void compute_forces_fluids (PSO_ARGS) {
    USE_GRID_PROPS

    USE_FIELD(density, double) USE_FIELD(force, double) USE_FIELD(position, double)
    USE_FIELD(velocity, double)

    USE_FIELD_FIRST_VALUE(n, uint) USE_FIELD_FIRST_VALUE(mass, double)
    USE_FIELD_FIRST_VALUE(smoothingradius, double) USE_FIELD_FIRST_VALUE(pnum, uint)
    USE_FIELD_FIRST_VALUE(restdens, double) USE_FIELD_FIRST_VALUE(stiffness, double)
    USE_FIELD_FIRST_VALUE(viscosity, double)
    
    unsigned int i = get_global_id(0);

    if (i >= n) return;

    double3 ipos = vload3(i, position);
    double3 ivel = vload3(i, velocity);

#define INIT_FLUIDS_FORCE_COMPUTATION \
    double3 f_i = (double3)(0, 0, 0);

    INIT_FLUIDS_FORCE_COMPUTATION

    FOR_PARTICLES_IN_RANGE(i, j,
        if (j == i) continue;

#define FLUIDS_FORCE_COMPUTATION \
        double p_i = (density[i] - restdens) * stiffness;\
        double p_j = (density[j] - restdens) * stiffness;\
\
        double3 jpos = vload3(j, position);\
        double3 jvel = vload3(j, velocity);\
\
        double3 diff = jpos - ipos;\
        double3 relvel = jvel - ivel;\
\
        double dist = length(diff);\
\
        if (dist > smoothingradius) continue;\
\
        double dist_in = smoothingradius - dist;\
\
        double didj = density[i] * density[j];\
\
        double sr6 = pow(smoothingradius, 6);\
\
        double vterm = viscosity * dist_in * 45 / PI / sr6;\
        double pterm = (p_i + p_j) * 0.5 / dist * dist_in * dist_in * (-45 / PI / sr6);\
\
        f_i += (diff * pterm + relvel * vterm) / didj;

        FLUIDS_FORCE_COMPUTATION
    )

#define FINALISE_FLUIDS_FORCE_COMPUTATION \
    f_i *= mass;

    FINALISE_FLUIDS_FORCE_COMPUTATION

    APPLY_CUBE_BOUNDS(ipos, f_i, -2.0, 2.0)

    vstore3(f_i, i, force);
}
