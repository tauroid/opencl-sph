inline void computeGreenStrain (float deformation[9], float gstrain[6]) {
    float ftf[9];
    float deformation_t[9];

    transpose(deformation, deformation_t);

    multiplyMatrices(deformation_t, deformation, ftf);

    gstrain[S_00] = 0.5*ftf[I_00] - 0.5;
    gstrain[S_11] = 0.5*ftf[I_11] - 0.5;
    gstrain[S_22] = 0.5*ftf[I_22] - 0.5;
    gstrain[S_01] = 0.5*ftf[I_01];
    gstrain[S_02] = 0.5*ftf[I_02];
    gstrain[S_12] = 0.5*ftf[I_12];
}
inline void computeSVKStress (float gstrain[6], float youngs_modulus, float poisson, float svk[6]) {
    float lame_lambda = youngs_modulus*poisson/(1.0+poisson)/(1.0-2.0*poisson);
    float lame_mu = youngs_modulus/2.0/(1.0+poisson);

    float firstterm = lame_lambda*(gstrain[S_00] + gstrain[S_11] + gstrain[S_22]);

    svk[S_00] = firstterm + 2.0*lame_mu*gstrain[S_00];
    svk[S_11] = firstterm + 2.0*lame_mu*gstrain[S_11];
    svk[S_22] = firstterm + 2.0*lame_mu*gstrain[S_22];
    svk[S_01] = 2.0*lame_mu*gstrain[S_01];
    svk[S_02] = 2.0*lame_mu*gstrain[S_02];
    svk[S_12] = 2.0*lame_mu*gstrain[S_12];
}
inline void SVKToCauchy (float svk[6], float deformation[9], float cauchy[6]) {
    float temp[9];

    for (uint d = 0; d < 9; ++d) temp[d] = 0;

    multiplyAS(deformation, svk, temp);

    float deformation_t[9];

    transpose(deformation, deformation_t);

    multiplyMatrices(temp, deformation_t, cauchy);

    multiplyMatrixScalar(1.0/determinant(deformation), cauchy, cauchy);
}

kernel void compute_stresses (PSO_ARGS) {
    USE_FIELD_FIRST_VALUE(n, uint)
    USE_FIELD_FIRST_VALUE(youngs_modulus, float)
    USE_FIELD_FIRST_VALUE(poisson,        float)

    USE_FIELD(deformation, float)
    USE_FIELD(stress,      float)

    uint i = get_global_id(0);

    if (i >= n) return;

    global float * def = deformation + i*3*3;

    float strain[6];
    
    computeGreenStrain(def, strain);

    float svk[6];

    computeSVKStress(strain, youngs_modulus, poisson, svk);

    float cauchy[6];
    
    SVKToCauchy(svk, def, cauchy);

    global float * s = stress + i*6;

    s[S_00] = cauchy[I_00];
    s[S_11] = cauchy[I_11];
    s[S_22] = cauchy[I_22];

    s[S_01] = cauchy[I_01];
    s[S_02] = cauchy[I_02];
    s[S_12] = cauchy[I_12];
}

kernel void compute_deformations (PSO_ARGS) {
    USE_GRID_PROPS

    USE_FIELD(deformation, float) USE_FIELD(position, float)
    USE_FIELD(originalpos, float) USE_FIELD(density, float)

    USE_FIELD_FIRST_VALUE(smoothingradius, float)
    USE_FIELD_FIRST_VALUE(mass, float)
    USE_FIELD_FIRST_VALUE(n, uint)

#ifdef SYMMETRY
    USE_FIELD(symmetry_planes, float)

    uint num_symmetry_planes = dimensions[dimensions_offsets[symmetry_planes_f] + num_dimensions[symmetry_planes_f] - 1];
#endif

    uint i = get_global_id(0);

    if (i >= n) return;

    float3 ipos = vload3(i, position);
    float3 ipos0 = vload3(i, originalpos);

    float apq[9];
    float aqq[9];

    for (uint d = 0; d < 9; ++d) apq[d] = 0;
    for (uint d = 0; d < 9; ++d) aqq[d] = 0;

    FOR_PARTICLES_IN_RANGE(i, j,
        float3 jpos = vload3(j, position);
        float3 jpos0 = vload3(j, originalpos);

        float3 p = jpos - ipos;
        float3 q = jpos0 - ipos0;

        float kernelp = applyKernel(length(p), smoothingradius);

        if (j != i) {
            contributeApq(p, q, kernelp, apq);
            contributeAqq(q, kernelp, aqq);
        }

#ifdef SYMMETRY
        for (uint plane_bits = 1; plane_bits < (1 << num_symmetry_planes); ++plane_bits) {
            float3 refpos = jpos;
            float3 refpos0 = jpos0;

            bool skip_region = false;

            global float * symmetry_plane;
            float3 point;
            float3 normal;

            for (uint s = 0; s < num_symmetry_planes; ++s) {
                if ((plane_bits & (1 << s)) == 0) continue;

                symmetry_plane = symmetry_planes + s*6;

                point = vload3(0, symmetry_plane);
                normal = vload3(1, symmetry_plane);

                float jplane_dist = fabs(dot(normal, jpos - point));
                if (fabs(dot(normal, ipos - point)) > smoothingradius || jplane_dist > smoothingradius || jplane_dist < 1.0e-5) {
                    skip_region = true;
                    break;
                }

                refpos = refpos - 2 * normal * dot(normal, refpos - point);
                refpos0 = refpos0 - 2 * normal * dot(normal, refpos0 - point);
            }

            if (skip_region) continue;

            p = refpos - ipos;
            q = refpos0 - ipos0;

            float dist = length(p);

            if (dist < smoothingradius) {
                kernelp = applyKernel(dist, smoothingradius);
                contributeApq(p, q, kernelp, apq);
                contributeAqq(q, kernelp, aqq);
            }
        }
#endif
    )

    global float * def = deformation + i*3*3;

    jacobiInvert(aqq, aqq);

    multiplyAS(apq, aqq, def);
}

#ifdef SYMMETRY
    // For StrainTest, discard the symmetry forces acting from the negative side of the test axis to get an accurate force reading
    #define TEST_AXIS_CUTOFF_FORCES
#endif

kernel void compute_forces_solids (PSO_ARGS) {
    USE_GRID_PROPS

    USE_FIELD_FIRST_VALUE(n, uint)

    USE_FIELD(originalpos, float) USE_FIELD_FIRST_VALUE(smoothingradius, float)
    USE_FIELD(stress, float) USE_FIELD_FIRST_VALUE(mass, float) USE_FIELD(density0, float)
    USE_FIELD(force, float) USE_FIELD(velocity, float)
    USE_FIELD_FIRST_VALUE(viscosity, float) USE_FIELD(density, float) USE_FIELD(position, float)

    USE_FIELD(body_number, uint)
    USE_FIELD(padding, uint)

#ifdef SYMMETRY
    USE_FIELD(symmetry_planes, float)

    uint num_symmetry_planes = dimensions[dimensions_offsets[symmetry_planes_f] + num_dimensions[symmetry_planes_f] - 1];
#endif

#ifdef TEST_AXIS_CUTOFF_FORCES
    USE_FIELD(cutoff_force, float)
#endif

#define EXPORT_COMPONENTS

#ifdef EXPORT_COMPONENTS
    USE_FIELD(force_elastic, float)
    USE_FIELD(force_viscous, float)
#endif

    uint i = get_global_id(0);

    if (i >= n) return;

    float3 ipos = vload3(i, position);
    float3 ivel = vload3(i, velocity);

#define INIT_SOLIDS_FORCE_COMPUTATION \
    float3 ipos0 = vload3(i, originalpos);\
\
    global float * i_stress = stress + i*6;\
\
    float3 f_e = (float3)(0, 0, 0);\
    float3 f_v = (float3)(0, 0, 0);

    INIT_SOLIDS_FORCE_COMPUTATION
#ifdef TEST_AXIS_CUTOFF_FORCES
    float3 f_ec = (float3)(0, 0, 0);
    float3 f_vc = (float3)(0, 0, 0);
#endif

    float3 jpos0, x, f_ji, f_ij, jpos, jvel;

    FOR_PARTICLES_IN_RANGE(i, j,
        jpos0 = vload3(j, originalpos);
        x = ipos0 - jpos0;
        jpos = vload3(j, position);
        jvel = vload3(j, velocity);

        float vivj = mass*mass/density0[i]/density0[j];

        global float * j_stress = stress + j*6;

        float3 fe = (float3)(0, 0, 0);
        float3 fv = (float3)(0, 0, 0);

#define SOLIDS_FORCE_COMPUTATION \
        f_ji = -vivj * multiplySymMatrixVector(i_stress, applyKernelGradient(x, smoothingradius));\
        f_ij = body_number[j] > 0 ? -f_ji : -vivj * multiplySymMatrixVector(j_stress, applyKernelGradient(-x, smoothingradius));\
        fe = -f_ji + f_ij;\
\
        fv = mass/density[j] * (jvel - ivel) * applyLapKernel(length(jpos - ipos), smoothingradius);\
\
        f_e += fe;\
        f_v += fv;

        if (j != i) {
            SOLIDS_FORCE_COMPUTATION
#ifdef TEST_AXIS_CUTOFF_FORCES
            f_ec += fe;
            f_vc += fv;
#endif
        }

#ifdef SYMMETRY
        // Go through every region created by the symmetry planes
        for (uint plane_bits = 1; plane_bits < (1 << num_symmetry_planes); ++plane_bits) {
            float3 refpos = jpos;
            float3 refpos0 = jpos0;
            float3 refipos0 = ipos0;

            bool skip_region = false;

            global float * symmetry_plane;
            float3 point;
            float3 normal;

            // Reflect by every "1" plane
            for (uint s = 0; s < num_symmetry_planes; ++s) {
                if ((plane_bits & (1 << s)) == 0) continue;

                symmetry_plane = symmetry_planes + s*6;

                point = vload3(0, symmetry_plane);
                normal = vload3(1, symmetry_plane);

                // Ignore this region if 
                //  - i is too far away from the boundary
                //  - j is too far away from the boundary
                //  - j is close enough to be regarded as on the boundary - then unreflected j is all that's needed
                float jplane_dist = fabs(dot(normal, jpos - point));
                if (fabs(dot(normal, ipos - point)) > smoothingradius || jplane_dist > smoothingradius || jplane_dist < 1.0e-5) {
                    skip_region = true;
                    break;
                }

                refpos = refpos - 2 * normal * dot(normal, refpos - point);
                refpos0 = refpos0 - 2 * normal * dot(normal, refpos0 - point);
                refipos0 = refipos0 - 2 * normal * dot(normal, refipos0 - point);
            }

            if (skip_region) continue;

            float3 diff = refpos - ipos;

            float dist = length(diff);

            x = ipos0 - refpos0;
            float3 refx = refipos0 - jpos0;

            f_ji = -vivj * multiplySymMatrixVector(i_stress, applyKernelGradient(x, smoothingradius));
            f_ij = -vivj * multiplySymMatrixVector(j_stress, applyKernelGradient(-refx, smoothingradius));

            for (uint s = num_symmetry_planes; s > 0; --s) {
                if ((plane_bits & (1 << (s-1))) == 0) continue;

                symmetry_plane = symmetry_planes + (s-1)*6;

                normal = vload3(1, symmetry_plane);

                f_ij = f_ij - 2 * normal * dot(normal, f_ij);
            }

            fe = -f_ji + f_ij;
            f_e += fe;

            float3 refvel = jvel - 2 * normal * dot(normal, jvel);
            
            fv = mass/density[j] * (refvel - ivel) * applyLapKernel(length(refpos - ipos), smoothingradius);
            f_v += fv;

#ifdef TEST_AXIS_CUTOFF_FORCES
            // Depends on test axis being zero
            if ((plane_bits & (1 << 0)) == 0) { // Still count the forces from the other mirrored planes
                f_ec += fe;
                f_vc += fv;
            }
#endif
        }
#endif
    )

#define FINALISE_SOLIDS_FORCE_COMPUTATION \
    f_e *= 0.5f;\
    f_v *= viscosity;

    FINALISE_SOLIDS_FORCE_COMPUTATION

    float3 f = f_e + f_v;

    //APPLY_CUBE_BOUNDS(ipos, f, -2.0f, 2.0f)

    vstore3(f, i, force);

#ifdef EXPORT_COMPONENTS
    vstore3(f_e, i, force_elastic);
    vstore3(f_v, i, force_viscous);
#endif

#ifdef TEST_AXIS_CUTOFF_FORCES
    f_ec *= 0.5f;
    f_vc *= viscosity;

    vstore3(f_ec + f_vc, i, cutoff_force);
#endif
}


