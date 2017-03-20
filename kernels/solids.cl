////////// Physics stuff //////////

inline void contributeApq (float3 p, float3 q, float weight, float apq[9]) {
    float3 wp = weight*p;

    apq[I_00] += wp.x*q.x; apq[I_01] += wp.x*q.y; apq[I_02] += wp.x*q.z;
    apq[I_10] += wp.y*q.x; apq[I_11] += wp.y*q.y; apq[I_12] += wp.y*q.z;
    apq[I_20] += wp.z*q.x; apq[I_21] += wp.z*q.y; apq[I_22] += wp.z*q.z;
}
inline void computeCauchyStrain (float deformation[9], global float strain[6]) {
    strain[S_00] = deformation[I_00];
    strain[S_01] = 0.5f*(deformation[I_01] + deformation[I_10]);
    strain[S_02] = 0.5f*(deformation[I_02] + deformation[I_20]);
    strain[S_11] = deformation[I_11];
    strain[S_12] = 0.5f*(deformation[I_12] + deformation[I_21]);
    strain[S_22] = deformation[I_22];
}
inline void computeStress (global float strain[6], float youngs_modulus, float poisson, global float stress[6]) {
    float lame_lambda = youngs_modulus*poisson/(1.0f+poisson)/(1-2.0f*poisson);
    float lame_mu = youngs_modulus/2.0f/(1.0f+poisson);

    float c[36] = {
        lame_lambda + 2.0f * lame_mu, lame_lambda              , lame_lambda              , 0          , 0          , 0          ,
        lame_lambda              , lame_lambda + 2.0f * lame_mu, lame_lambda              , 0          , 0          , 0          ,
        lame_lambda              , lame_lambda              , lame_lambda + 2.0f * lame_mu, 0          , 0          , 0          ,
        0                        , 0                        , 0                        , 2.0f * lame_mu, 0          , 0          ,
        0                        , 0                        , 0                        , 0          , 2.0f * lame_mu, 0          ,
        0                        , 0                        , 0                        , 0          , 0          , 2.0f * lame_mu,
    };
    
    for (uint i = 0; i < 6; ++i) {
        uint ir = i*6;
        stress[i] = c[ir]*strain[0] + c[ir+1]*strain[1] + c[ir+2]*strain[2] + c[ir+3]*strain[3] + c[ir+4]*strain[4] + c[ir+5]*strain[5];
    }
}

////////// Particle creation and transformation //////////

kernel void init_original_position (PSO_ARGS) {
    USE_FIELD(position, float) USE_FIELD(originalpos, float)
    USE_FIELD_FIRST_VALUE(n, uint)

    unsigned int i = get_global_id(0);

    if (i >= n) return;

    float3 pos = vload3(i, position);

    vstore3(pos, i, originalpos);
}

////////// Kernels //////////

kernel void compute_rotations_and_strains (PSO_ARGS) {
    USE_GRID_PROPS

    USE_FIELD_FIRST_VALUE(n, uint)
    
    USE_FIELD(rotation, float) USE_FIELD(strain, float) USE_FIELD(position, float)
    USE_FIELD(originalpos, float) USE_FIELD_FIRST_VALUE(smoothingradius, float)
    USE_FIELD(density0, float) USE_FIELD_FIRST_VALUE(mass, float)
    USE_FIELD(density, float)

    USE_FIELD(body_number, uint)

    USE_FIELD(intermediate, float)
    USE_FIELD(apq_temp, float)

    uint i = get_global_id(0);

    if (i >= n || body_number[i] > 0) return;

    float3 ipos = vload3(i, position);
    float3 ipos0 = vload3(i, originalpos);

    global float * apq = apq_temp + i*3*3;

    for (uint d = 0; d < 9; ++d) apq[d] = 0;

    FOR_PARTICLES_IN_RANGE(i, j,
        if (j == i) continue;

        float3 p = vload3(j, position) - ipos;
        float3 q = vload3(j, originalpos) - ipos0;

        contributeApq(p, q, applyKernel(length(p), smoothingradius), apq);
    )

    global float * r = rotation + i*3*3;

    global float * inter = intermediate + i*3*3;

    for (uint d = 0; d < 9; ++d) inter[d] = apq[d];

    getR(inter, r);

    float r_t[9];

    transpose(r, r_t);

    float deformation[9];

    for (uint d = 0; d < 9; ++d) deformation[d] = 0;

    FOR_PARTICLES_IN_RANGE(i, j,
        if (j == i) continue;

        float3 p = vload3(j, position) - ipos;
        p = multiplyMatrixVector(r_t, p);

        float3 q = vload3(j, originalpos) - ipos0;

        float3 u = p-q;

        addOuterProduct(u*mass/density0[i], applyKernelGradient(q, smoothingradius), deformation);
    )

    global float * s = strain + i*6;

    zeroArray(s, 6);
    
    computeCauchyStrain(deformation, s);
}
kernel void compute_original_density (PSO_ARGS) {
    USE_GRID_PROPS

    USE_FIELD_FIRST_VALUE(pnum, uint) USE_FIELD_FIRST_VALUE(n, uint) USE_FIELD_FIRST_VALUE(mass, float)
    USE_FIELD(position, float) USE_FIELD(density0, float) USE_FIELD_FIRST_VALUE(smoothingradius, float)

    unsigned int i = get_global_id(0);

    if (i >= n) return;

    float3 ipos = vload3(i, position);

    density0[i] = 0;

    FOR_PARTICLES_IN_RANGE(i, j,
        j = cellparticles[jp];

        float3 jpos = vload3(j, position);

        float3 diff = jpos - ipos;

        float dist = length(diff);

        density0[i] += applyKernel(dist, smoothingradius);
    )

    density0[i] *= mass;
}
kernel void compute_stresses (PSO_ARGS) {
    USE_FIELD(strain, float) USE_FIELD(stress, float)
    USE_FIELD_FIRST_VALUE(youngs_modulus, float) USE_FIELD_FIRST_VALUE(poisson, float)

    USE_FIELD_FIRST_VALUE(n, uint)

    unsigned int i = get_global_id(0);

    if (i >= n) return;

    computeStress(strain + i*6, youngs_modulus, poisson, stress + i*6);
}
   
kernel void compute_forces_solids (PSO_ARGS) {
    USE_GRID_PROPS

    USE_FIELD_FIRST_VALUE(n, uint)

    USE_FIELD(originalpos, float) USE_FIELD_FIRST_VALUE(smoothingradius, float)
    USE_FIELD(stress, float) USE_FIELD_FIRST_VALUE(mass, float) USE_FIELD(density0, float)
    USE_FIELD(rotation, float) USE_FIELD(force, float) USE_FIELD(velocity, float)
    USE_FIELD_FIRST_VALUE(viscosity, float) USE_FIELD(density, float) USE_FIELD(position, float)

    USE_FIELD(body_number, uint)

    uint i = get_global_id(0);

    if (i >= n) return;

    float3 ipos = vload3(i, position);
    float3 ivel = vload3(i, velocity);

// Repurpose for later batches
#define INIT_SOLIDS_FORCE_COMPUTATION \
    float3 ipos0 = vload3(i, originalpos);\
\
    global float * i_stress = stress + i*6;\
    global float * i_rotation = rotation + i*3*3;\
\
    float3 f_e = (float3)(0, 0, 0);\
    float3 f_v = (float3)(0, 0, 0);

    INIT_SOLIDS_FORCE_COMPUTATION

    FOR_PARTICLES_IN_RANGE(i, j,
        if (j == i) continue;

#define SOLIDS_FORCE_COMPUTATION \
        float3 jpos0 = vload3(j, originalpos);\
\
        global float * j_stress = stress + j*6;\
\
        float3 x = jpos0 - ipos0;\
        float vivj = mass*mass/density0[i]/density0[j];\
        float3 f_ji = -vivj * multiplySymMatrixVector(i_stress, applyKernelGradient(x, smoothingradius));\
        float3 f_ij = body_number[j] > 0 ? -f_ji : -vivj * multiplySymMatrixVector(j_stress, applyKernelGradient(-x, smoothingradius));\
\
        global float * j_rotation = rotation + j*3*3;\
\
        f_e += -multiplyMatrixVector(i_rotation, f_ji) + multiplyMatrixVector(j_rotation, f_ij);\
\
        float3 jpos = vload3(j, position);\
        float3 jvel = vload3(j, velocity);\
\
        f_v += mass/density[j] * (jvel - ivel) * applyLapKernel(length(jpos - ipos), smoothingradius);

        SOLIDS_FORCE_COMPUTATION
    )

#define FINALISE_SOLIDS_FORCE_COMPUTATION \
    f_e *= 0.5f;\
    f_v *= viscosity;

    FINALISE_SOLIDS_FORCE_COMPUTATION
    
    float3 f = f_e + f_v;

    //APPLY_CUBE_BOUNDS(ipos, f, -2.0f, 2.0f)

    vstore3(f, i, force);
}
