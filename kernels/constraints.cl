kernel void apply_plane_constraints(PSO_ARGS) {
    USE_FIELD(position, float) USE_FIELD(velocity, float)
    USE_FIELD(plane_constraints, float) USE_FIELD(plane_constraints_particles, uint)

    USE_FIELD_FIRST_VALUE(n, uint)

    uint i = get_global_id(0);

    if (i >= n || plane_constraints_particles[i] == 0) return;

    global float * plane_constraint = plane_constraints + (plane_constraints_particles[i]-1)*6;

    float3 point = vload3(0, plane_constraint);
    float3 normal = vload3(1, plane_constraint);

    float3 ipos = vload3(i, position);
    float3 ivel = vload3(i, velocity);

    ipos = ipos - dot(normal, ipos - point) * normal;
    ivel = ivel - dot(normal, ivel) * normal;

    vstore3(ipos, i, position);
    vstore3(ivel, i, velocity);
}
