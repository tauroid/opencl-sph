kernel void apply_plane_constraints(PSO_ARGS) {
    USE_FIELD(position, double) USE_FIELD(velocity, double)
    USE_FIELD(plane_constraints, double) USE_FIELD(plane_constraints_particles, uint)

    USE_FIELD_FIRST_VALUE(n, uint)

    uint i = get_global_id(0);

    if (i >= n || plane_constraints_particles[i] == 0) return;

    global double * plane_constraint = plane_constraints + (plane_constraints_particles[i]-1)*6;

    double3 point = vload3(0, plane_constraint);
    double3 normal = vload3(1, plane_constraint);

    double3 ipos = vload3(i, position);
    double3 ivel = vload3(i, velocity);

    ipos = ipos - dot(normal, ipos - point) * normal;
    ivel = ivel - dot(normal, ivel) * normal;

    vstore3(ipos, i, position);
    vstore3(ivel, i, velocity);
}
