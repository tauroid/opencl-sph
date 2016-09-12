#pragma OPENCL EXTENSION cl_khr_fp64 : enable

#define USE_FIELD(name, type) global type * name = (global type *) name##_m;
#define USE_FIELD_FIRST_VALUE(name, type) private type name = *((global type *) name##_m);

#define PSO_ARGS uint num_fields,\
                global char * names,         global uint * names_offsets,\
                global uint * dimensions,    global uint * num_dimensions, global uint * dimensions_offsets, global uint * entry_sizes,\
                global void * data,          global uint * data_sizes,     global uint * data_offsets

#define PI 3.1415926535

kernel void find_particle_bins (PSO_ARGS) {
    USE_FIELD(position, double) USE_FIELD(gridbounds, double)
    USE_FIELD(gridres, uint)    USE_FIELD(gridcell, uint) USE_FIELD(gridcount, uint)
    USE_FIELD_FIRST_VALUE(pnum, uint) USE_FIELD_FIRST_VALUE(n, uint)
    USE_FIELD_FIRST_VALUE(smoothingradius, double)

    size_t i = get_global_id(0);

    if (i >= n) return;

    double3 pos = (double3) (position[i*3], position[i*3 + 1], position[i*3 + 2]);

    uint3 gc = clamp((uint3) ((pos.x - gridbounds[0]) / (gridbounds[1] - gridbounds[0]) * gridres[0],
                              (pos.y - gridbounds[2]) / (gridbounds[3] - gridbounds[2]) * gridres[1],
                              (pos.z - gridbounds[4]) / (gridbounds[5] - gridbounds[4]) * gridres[2]),
                     (uint3) (0, 0, 0),
                     (uint3) (gridres[0]-1, gridres[1]-1, gridres[2]-1));

    gridcell[i] = gc.z * gridres[0] * gridres[1] + gc.y * gridres[0] + gc.x;

    gridcount[i] = 0;
}

int get_cell_at_offset (global uint * gridres, uint original, int x, int y, int z) {
    uint rowlength = gridres[0];
    uint layersize = gridres[0] * gridres[1];

    uint zo = original / layersize;
    uint yo = (original - layersize * zo) / rowlength;
    uint xo = original - layersize * zo - rowlength * yo;

    int zn = zo + z;
    int yn = yo + y;
    int xn = xo + x;

    if (xn >= gridres[0] || xn < 0 ||
        yn >= gridres[1] || yn < 0 ||
        zn >= gridres[2] || zn < 0) return -1;

    return zn * layersize + yn * rowlength + xn;
}

kernel void count_particles_in_bins (PSO_ARGS) {
    USE_FIELD(gridcell, uint) USE_FIELD(gridcount, uint) USE_FIELD_FIRST_VALUE(n, uint)
    size_t i = get_global_id(0);

    if (i >= n) return;

    atom_inc (&gridcount[gridcell[i]]);
}

kernel void prefix_sum (PSO_ARGS, local uint * temp, uint array_size, global uint * block_totals, uint num_blocks) {
    USE_FIELD(gridcount, uint) USE_FIELD(celloffset, uint) USE_FIELD(gridres, uint)

    size_t i_global = get_global_id(0);
    size_t i_local = get_local_id(0);

    /* Find bloody block we're in, binary search */
    /*
    if (num_blocks == 1) {
        s = 0;
    } else {
        s = num_blocks / 2;

        uint min_index = 0;
        uint max_index = num_blocks;

        int repeat = 1;
        while (repeat) {
            if (i_global < block_indices[s]) {
                max_index = s;
                s = (s + min_index) / 2;
            } else if (i_global >= block_indices[s+1]) {
                min_index = s+1;
                s = (s + max_index) / 2;
            } else {
                repeat = 0;
            }

            if (s == num_blocks - 1 || s == 0) repeat = 0;
        }
    }
    */

    size_t i = get_local_id(0);
    size_t n = get_local_size(0)*2;
    size_t g_id = get_group_id(0);
    size_t global_offset = get_group_id(0) * get_local_size(0) * 2;

    int in_array_1 = i + global_offset < array_size;
    int in_array_2 = i + n/2 + global_offset < array_size;

    /* Copy array to temp memory */
    if (in_array_1) temp[i] = gridcount[i + global_offset];
    else temp[i] = 0;
    if (in_array_2) temp[i + n/2] = gridcount[i + n/2 + global_offset];
    else temp[i + n/2] = 0;

    uint last_element_count;
    if (i == n/2 - 1) {
        last_element_count = temp[i + n/2];
    }

    /* Begin black magic */
    /* i.e. do the prefix sum on temp */
    uint stride = 1;
    for (uint d = n>>1; d > 0; d >>= 1) {
        barrier(CLK_LOCAL_MEM_FENCE);

        if (i < d) {
            uint ai = stride*(2*i+1) - 1;
            uint bi = ai + stride;

            temp[bi] += temp[ai];
        }

        stride <<= 1;
    }
    barrier(CLK_LOCAL_MEM_FENCE);
     
    if (i == 0) temp[n - 1] = 0;

    for (uint d = 1; d < n; d <<= 1) {
        barrier(CLK_LOCAL_MEM_FENCE);

        stride >>= 1;

        if (i < d) {
            uint ai = stride*(2*i+1) - 1;
            uint bi = ai + stride;

            uint t = temp[bi];
            temp[bi] += temp[ai];
            temp[ai] = t;
        }
    }
    /* End black magic */

    barrier(CLK_LOCAL_MEM_FENCE);

    if (in_array_1) celloffset[i + global_offset] = temp[i];
    if (in_array_2) celloffset[i + n/2 + global_offset] = temp[i + n/2];

    if (i == n/2 - 1) {
        block_totals[g_id] = temp[i + n/2] + last_element_count;
        uint g_id_c;
        for (g_id_c = g_id+1; g_id_c < num_blocks; ++g_id_c) {
            block_totals[g_id_c] += block_totals[g_id];
        }
    }

    barrier(CLK_GLOBAL_MEM_FENCE);

    if (g_id > 0) {
        if (in_array_1) celloffset[i + global_offset] += block_totals[g_id-1];
        if (in_array_2) celloffset[i + n/2 + global_offset] += block_totals[g_id-1];
    }
}

kernel void copy_celloffset_to_backup(PSO_ARGS, global uint * backup_prefix_sum, uint num_grid_cells) {
    USE_FIELD(celloffset, uint)

    size_t i = get_global_id(0);

    if (i < num_grid_cells) backup_prefix_sum[i] = celloffset[i];
}

kernel void insert_particles_in_bin_array (PSO_ARGS, global uint * backup_prefix_sum) {
    USE_FIELD_FIRST_VALUE(n, uint) USE_FIELD(gridcell, uint) USE_FIELD(cellparticles, uint)

    size_t i = get_global_id(0);

    if (i < n) cellparticles[atomic_inc(backup_prefix_sum + gridcell[i])] = i;
}

kernel void populate_position_cuboid (PSO_ARGS, double3 corner1, double3 corner2, uint3 size) {
    if (get_work_dim() < 3) return;

    size_t i = get_global_id(0);
    size_t j = get_global_id(1);
    size_t k = get_global_id(2);

    if (i >= size.x || j >= size.y || k >= size.z) return;

    USE_FIELD(position, double) USE_FIELD_FIRST_VALUE(pnum, uint) USE_FIELD(n, uint)

    if (i == size.x - 1 && j == size.y - 1 && k == size.z - 1) n[0] = size.x*size.y*size.z;

    int p_ix = i + size.x*j + size.x*size.y*k;
    double3 p = (double3) (((double) i/ (double) (size.x-1)) * (corner2.x - corner1.x) + corner1.x,
                           ((double) j/ (double) (size.y-1)) * (corner2.y - corner1.y) + corner1.y,
                           ((double) k/ (double) (size.z-1)) * (corner2.z - corner1.z) + corner1.z);

    vstore3(p, p_ix, position);
}

double apply_kernel (double smoothingradius, double dist) {
    return dist > smoothingradius ? 0 : 315/64/PI/pow(smoothingradius, 9) * pow(pow(smoothingradius, 2) - pow(dist, 2), 3);
}

double apply_diff_kernel (double smoothingradius, double dist) {
    return dist > smoothingradius ? 0 : -1890/64/PI/pow(smoothingradius, 9) * dist * pow(pow(smoothingradius, 2) - pow(dist, 2), 3);
}

kernel void compute_density (PSO_ARGS) {
    USE_FIELD(gridcell,      uint) USE_FIELD(gridcount, uint) USE_FIELD(celloffset, uint)
    USE_FIELD(cellparticles, uint) USE_FIELD(gridres,   uint)

    USE_FIELD_FIRST_VALUE(pnum, uint) USE_FIELD_FIRST_VALUE(n, uint) USE_FIELD_FIRST_VALUE(mass, double)
    USE_FIELD(position, double) USE_FIELD(density, double) USE_FIELD_FIRST_VALUE(smoothingradius, double)

    unsigned int i = get_global_id(0);

    if (i >= n) return;

    double3 ipos = vload3(i, position);

    density[i] = 0;

    uint this_grid_cell = gridcell[i];

    int gx, gy, gz;
    uint jp, j;

    for (gz = -1; gz <= 1; ++gz)
    for (gy = -1; gy <= 1; ++gy)
    for (gx = -1; gx <= 1; ++gx) {
        int cell = get_cell_at_offset(gridres, this_grid_cell, gx, gy, gz);
        uint offset = celloffset[cell];

        if (cell == -1) continue;
        for (jp = offset; jp < offset + gridcount[cell]; ++jp) {
            j = cellparticles[jp];

            double3 jpos = vload3(j, position);

            double3 diff = jpos - ipos;

            double dist = length(diff);

            density[i] += apply_kernel(smoothingradius, dist);
        }
    }

    density[i] *= mass;
}

kernel void compute_forces (PSO_ARGS) {
    USE_FIELD(gridcell,      uint) USE_FIELD(gridcount, uint) USE_FIELD(celloffset, uint)
    USE_FIELD(cellparticles, uint) USE_FIELD(gridres,   uint)

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

    uint this_grid_cell = gridcell[i];

    int gx, gy, gz;
    uint jp, j;

    double3 f_i = (double3)(0, 0, 0);

    for (gz = -1; gz <= 1; ++gz)
    for (gy = -1; gy <= 1; ++gy)
    for (gx = -1; gx <= 1; ++gx) {
        int cell = get_cell_at_offset(gridres, this_grid_cell, gx, gy, gz);
        uint offset = celloffset[cell];

        if (cell == -1) continue;

        for (jp = offset; jp < offset + gridcount[cell]; ++jp) {
            j = cellparticles[jp];

            if (j == i) continue;

            double p_i = (density[i] - restdens) * stiffness;
            double p_j = (density[j] - restdens) * stiffness;

            double3 jpos = vload3(j, position);
            double3 jvel = vload3(j, velocity);

            double3 diff = jpos - ipos;
            double3 relvel = jvel - ivel;

            double dist = length(diff);

            if (dist > smoothingradius) continue;

            double dist_in = smoothingradius - dist;

            double didj = density[i] * density[j];

            double sr6 = pow(smoothingradius, 6);

            double vterm = viscosity * dist_in * 45 / PI / sr6;
            double pterm = (p_i + p_j) * 0.5 / dist * dist_in * dist_in * (-45 / PI / sr6);

            // Following fluidsv3, just using the same poly6 kernel instead of the faster ones
            f_i += (diff * pterm + relvel * vterm) / didj;
        }
    }

    f_i *= mass;

    vstore3(f_i, i, force);
}

kernel void step_forward (PSO_ARGS) {
    USE_FIELD_FIRST_VALUE(mass, double) USE_FIELD_FIRST_VALUE(timestep, double)
    USE_FIELD_FIRST_VALUE(n, uint)

    USE_FIELD(acceleration, double) USE_FIELD(force, double) USE_FIELD(position, double)
    USE_FIELD(velocity, double) USE_FIELD(veleval, double) USE_FIELD(posnext, double)

    uint i = get_global_id(0);

    if (i >= n) return;

    double3 f = vload3(i, force);
    double3 a = f / mass;

    vstore3(a, i, acceleration);

    double3 v = vload3(i, velocity);
    double3 vnext = v + timestep * a;
    double3 veval = (v + vnext) / 2;

    double3 p = vload3(i, position);

    double3 pnext = p + timestep * vnext;

    vstore3(pnext, i, position);
    vstore3(vnext, i, velocity);
}
