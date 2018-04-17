#ifndef OPENCL_SPH_REAL_TYPE
#define OPENCL_SPH_REAL_TYPE float
#endif

typedef OPENCL_SPH_REAL_TYPE REAL;
#if OPENCL_SPH_REAL_TYPE == float
typedef float2 REAL2;
typedef float3 REAL3;
typedef float4 REAL4;
#elif OPENCL_SPH_REAL_TYPE == double
typedef double2 REAL2;
typedef double3 REAL3;
typedef double4 REAL4;
#else
#error "OPENCL_SPH_REAL_TYPE must be either float or double."
#endif

kernel void compute_forces_fluids (PSO_ARGS) {
    USE_GRID_PROPS

    USE_FIELD(density, REAL) USE_FIELD(force, REAL) USE_FIELD(position, REAL)
    USE_FIELD(velocity, REAL)

    USE_FIELD_FIRST_VALUE(n, uint) USE_FIELD_FIRST_VALUE(mass, REAL)
    USE_FIELD_FIRST_VALUE(smoothingradius, REAL) USE_FIELD_FIRST_VALUE(pnum, uint)
    USE_FIELD_FIRST_VALUE(restdens, REAL) USE_FIELD_FIRST_VALUE(stiffness, REAL)
    USE_FIELD_FIRST_VALUE(viscosity, REAL)
    
    unsigned int i = get_global_id(0);

    if (i >= n) return;

    REAL3 ipos = vload3(i, position);
    REAL3 ivel = vload3(i, velocity);

#define INIT_FLUIDS_FORCE_COMPUTATION \
    REAL3 f_i = (REAL3)(0, 0, 0);

    INIT_FLUIDS_FORCE_COMPUTATION

    FOR_PARTICLES_IN_RANGE(i, j,
        if (j == i) continue;

#define FLUIDS_FORCE_COMPUTATION \
        REAL p_i = (density[i] - restdens) * stiffness;\
        REAL p_j = (density[j] - restdens) * stiffness;\
\
        REAL3 jpos = vload3(j, position);\
        REAL3 jvel = vload3(j, velocity);\
\
        REAL3 diff = jpos - ipos;\
        REAL3 relvel = jvel - ivel;\
\
        REAL dist = length(diff);\
\
        if (dist > smoothingradius) continue;\
\
        REAL dist_in = smoothingradius - dist;\
\
        REAL didj = density[i] * density[j];\
\
        REAL sr6 = pow(smoothingradius, 6);\
\
        REAL vterm = viscosity * dist_in * 45 / PI / sr6;\
        REAL pterm = (p_i + p_j) * 0.5 / dist * dist_in * dist_in * (-45 / PI / sr6);\
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
