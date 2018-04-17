inline void computeCauchyStrain (float deformation[9], global float strain[6]) {
    strain[S_00] = deformation[I_00];
    strain[S_01] = 0.5f*(deformation[I_01] + deformation[I_10]);
    strain[S_02] = 0.5f*(deformation[I_02] + deformation[I_20]);
    strain[S_11] = deformation[I_11];
    strain[S_12] = 0.5f*(deformation[I_12] + deformation[I_21]);
    strain[S_22] = deformation[I_22];
}
inline void computeStress (global float cstrain[6], float youngs_modulus, float poisson, global float stress[6]) {
    float lame_lambda = youngs_modulus*poisson/(1.0f+poisson)/(1-2.0f*poisson);
    float lame_mu = youngs_modulus/2.0f/(1.0f+poisson);

    float c[36] = {
        lame_lambda + 2.0f * lame_mu, lame_lambda                 , lame_lambda                 , 0             , 0             , 0          ,
        lame_lambda                 , lame_lambda + 2.0f * lame_mu, lame_lambda                 , 0             , 0             , 0          ,
        lame_lambda                 , lame_lambda                 , lame_lambda + 2.0f * lame_mu, 0             , 0             , 0          ,
        0                           , 0                           , 0                           , 2.0f * lame_mu, 0             , 0          ,
        0                           , 0                           , 0                           , 0             , 2.0f * lame_mu, 0          ,
        0                           , 0                           , 0                           , 0             , 0             , 2.0f * lame_mu,
    };
    
    for (uint i = 0; i < 6; ++i) {
        uint ir = i*6;
        stress[i] = c[ir]*cstrain[0] + c[ir+1]*cstrain[1] + c[ir+2]*cstrain[2] + c[ir+3]*cstrain[3] + c[ir+4]*cstrain[4] + c[ir+5]*cstrain[5];
    }
}

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

#ifdef SYMMETRY
    USE_FIELD(symmetry_planes, float)

    uint num_symmetry_planes = dimensions[dimensions_offsets[symmetry_planes_f] + num_dimensions[symmetry_planes_f] - 1];
#endif

    uint i = get_global_id(0);

    if (i >= n || body_number[i] > 0) return;

    float3 ipos = vload3(i, position);
    float3 ipos0 = vload3(i, originalpos);

    global float * apq = apq_temp + i*3*3;

    for (uint d = 0; d < 9; ++d) apq[d] = 0;

    FOR_PARTICLES_IN_RANGE(i, j,
        float3 jpos = vload3(j, position);
        float3 jpos0 = vload3(j, originalpos);

        float3 p = jpos - ipos;
        float3 q = jpos0 - ipos0;

        if (j != i) contributeApq(p, q, applyKernel(length(p), smoothingradius), apq);

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
                contributeApq(p, q, applyKernel(dist, smoothingradius), apq);
            }
        }
#endif
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
        float3 jpos = vload3(j, position);
        float3 jpos0 = vload3(j, originalpos);

        float3 p = jpos - ipos;
        p = multiplyMatrixVector(r_t, p);

        float3 q = jpos0 - ipos0;

        float3 u = p-q;

        if (j != i) {
            addOuterProduct(u*mass/density0[i], applyKernelGradient(-q, smoothingradius), deformation);
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
            p = multiplyMatrixVector(r_t, p);

            q = refpos0 - ipos0;

            u = p-q;

            float dist = length(p);

            if (dist < smoothingradius) {
                addOuterProduct(u*mass/density0[i], applyKernelGradient(-q, smoothingradius), deformation);
            }
        }
#endif
    )

    global float * s = strain + i*6;

    zeroArray(s, 6);
    
    computeCauchyStrain(deformation, s);
}

kernel void compute_stresses (PSO_ARGS) {
    USE_FIELD(strain, float) USE_FIELD(stress, float)
    USE_FIELD_FIRST_VALUE(youngs_modulus, float) USE_FIELD_FIRST_VALUE(poisson, float)

    USE_FIELD_FIRST_VALUE(n, uint)

    unsigned int i = get_global_id(0);

    if (i >= n) return;

    computeStress(strain + i*6, youngs_modulus, poisson, stress + i*6);
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
    USE_FIELD(rotation, float) USE_FIELD(force, float) USE_FIELD(velocity, float)
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
    global float * i_rotation = rotation + i*3*3;\
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
        global float * j_rotation = rotation + j*3*3;

        float3 fe = (float3)(0, 0, 0);
        float3 fv = (float3)(0, 0, 0);

#define SOLIDS_FORCE_COMPUTATION \
        f_ji = -vivj * multiplySymMatrixVector(i_stress, applyKernelGradient(x, smoothingradius));\
        f_ij = body_number[j] > 0 ? -f_ji : -vivj * multiplySymMatrixVector(j_stress, applyKernelGradient(-x, smoothingradius));\
        fe = -multiplyMatrixVector(i_rotation, f_ji) + multiplyMatrixVector(j_rotation, f_ij);\
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
            f_ij = multiplyMatrixVector(j_rotation, f_ij);

            for (uint s = num_symmetry_planes; s > 0; --s) {
                if ((plane_bits & (1 << (s-1))) == 0) continue;

                symmetry_plane = symmetry_planes + (s-1)*6;

                normal = vload3(1, symmetry_plane);

                f_ij = f_ij - 2 * normal * dot(normal, f_ij);
            }

            fe = -multiplyMatrixVector(i_rotation, f_ji) + f_ij;
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


