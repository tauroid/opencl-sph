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

    double3 f_i = (double3)(0, 0, 0);

    FOR_PARTICLES_IN_RANGE(i, j,
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

        f_i += (diff * pterm + relvel * vterm) / didj;
    )

    f_i *= mass;

    // Hacky bounds, keep these inside the grid bounds for efficiency
    if (ipos.x < -1.3) f_i.x += 1/(1.5+ipos.x) - 5;
    else if (ipos.x > 1.3) f_i.x -= 1/(1.5-ipos.x) - 5;
    if (ipos.y < -1.3) f_i.y += 1/(1.5+ipos.y) - 5;
    else if (ipos.y > 1.3) f_i.y -= 1/(1.5-ipos.y) - 5;
    if (ipos.z < -1.3) f_i.z += 1/(1.5+ipos.z) - 5;
    else if (ipos.z > 1.3) f_i.z -= 1/(1.5-ipos.z) - 5;

    vstore3(f_i, i, force);
}
