kernel void compute_forces_fluids (PSO_ARGS) {
    USE_GRID_PROPS

    USE_FIELD(density, float) USE_FIELD(force, float) USE_FIELD(position, float)
    USE_FIELD(velocity, float)

    USE_FIELD_FIRST_VALUE(n, uint) USE_FIELD_FIRST_VALUE(mass, float)
    USE_FIELD_FIRST_VALUE(smoothingradius, float) USE_FIELD_FIRST_VALUE(pnum, uint)
    USE_FIELD_FIRST_VALUE(restdens, float) USE_FIELD_FIRST_VALUE(stiffness, float)
    USE_FIELD_FIRST_VALUE(viscosity, float)
    
    unsigned int i = get_global_id(0);

    if (i >= n) return;

    float3 ipos = vload3(i, position);
    float3 ivel = vload3(i, velocity);

#define INIT_FLUIDS_FORCE_COMPUTATION \
    float3 f_i = (float3)(0, 0, 0);

    INIT_FLUIDS_FORCE_COMPUTATION

    FOR_PARTICLES_IN_RANGE(i, j,
        if (j == i) continue;

#define FLUIDS_FORCE_COMPUTATION \
        float p_i = (density[i] - restdens) * stiffness;\
        float p_j = (density[j] - restdens) * stiffness;\
\
        float3 jpos = vload3(j, position);\
        float3 jvel = vload3(j, velocity);\
\
        float3 diff = jpos - ipos;\
        float3 relvel = jvel - ivel;\
\
        float dist = length(diff);\
\
        if (dist > smoothingradius) continue;\
\
        float dist_in = smoothingradius - dist;\
\
        float didj = density[i] * density[j];\
\
        float sr6 = pow(smoothingradius, 6);\
\
        float vterm = viscosity * dist_in * 45 / PI / sr6;\
        float pterm = (p_i + p_j) * 0.5 / dist * dist_in * dist_in * (-45 / PI / sr6);\
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
