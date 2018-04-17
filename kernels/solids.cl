////////// Physics stuff //////////

inline void contributeApq (float3 p, float3 q, float weight, float apq[9]) {
    float3 wp = weight*p;

    apq[I_00] += wp.x*q.x; apq[I_01] += wp.x*q.y; apq[I_02] += wp.x*q.z;
    apq[I_10] += wp.y*q.x; apq[I_11] += wp.y*q.y; apq[I_12] += wp.y*q.z;
    apq[I_20] += wp.z*q.x; apq[I_21] += wp.z*q.y; apq[I_22] += wp.z*q.z;
}

inline void contributeAqq (float3 q, float weight, float aqq[6]) {
    float3 wq = weight*q;

      aqq[S_00] += wq.x*q.x;   aqq[S_01] += wq.x*q.y;   aqq[S_02] += wq.x*q.z;
    /*aqq[S_10] += wq.y*q.x;*/ aqq[S_11] += wq.y*q.y;   aqq[S_12] += wq.y*q.z;
    /*aqq[S_20] += wq.z*q.x;   aqq[S_21] += wq.z*q.y;*/ aqq[S_22] += wq.z*q.z;
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

kernel void compute_original_density (PSO_ARGS) {
    USE_GRID_PROPS

    USE_FIELD_FIRST_VALUE(pnum, uint) USE_FIELD_FIRST_VALUE(n, uint) USE_FIELD_FIRST_VALUE(mass, float)
    USE_FIELD(position, float) USE_FIELD(density0, float) USE_FIELD_FIRST_VALUE(smoothingradius, float)

#ifdef SYMMETRY
    USE_FIELD(symmetry_planes, float)

    uint num_symmetry_planes = dimensions[dimensions_offsets[symmetry_planes_f] + num_dimensions[symmetry_planes_f] - 1];
#endif

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

#ifdef SYMMETRY
        // This only works when the symmetry planes are orthogonal
        for (uint plane_bits = 1; plane_bits < (1 << num_symmetry_planes); ++plane_bits) {
            float3 refpos = jpos;

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
            }

            if (skip_region) continue;

            diff = refpos - ipos;

            dist = length(diff);

            if (dist < smoothingradius) density0[i] += applyKernel(dist, smoothingradius);
        }
#endif
    )

    density0[i] *= mass;
}


