kernel void set_body_position(PSO_ARGS, uint body, float3 new_body_pos) {
    USE_FIELD_FIRST_VALUE(n, uint)      USE_FIELD_FIRST_VALUE(body_count, uint)
    USE_FIELD_FIRST_VALUE(timestep, float)

    USE_FIELD(body_number, uint)
    USE_FIELD(body_position, float)

    USE_FIELD(position, float)
    USE_FIELD(velocity, float)

    uint i = get_global_id(0);

    if (i >= n || body_number[i] != body+1 || body >= body_count) {
        barrier(CLK_GLOBAL_MEM_FENCE);

        if (i == 0) vstore3(new_body_pos, body, body_position);
        return;
    }

    float3 pos = vload3(i, position);
    float3 old_body_pos = vload3(body, body_position);

    float3 relative_pos = pos - old_body_pos;

    float3 new_pos = relative_pos + new_body_pos;

    vstore3(new_pos, i, position);
    vstore3((new_body_pos - old_body_pos) / timestep, i, velocity);

    barrier(CLK_GLOBAL_MEM_FENCE);

    if (i == 0) vstore3(new_body_pos, body, body_position);
}

void get_matrix_from_euler(float3 euler_angles, float output[9]) {
    float angle_x = euler_angles.x, angle_y = euler_angles.y, angle_x2 = euler_angles.z;

    // XYX
    float rotation_x [9];
    float rotation_y [9];
    float rotation_x2[9];

    float c_x = cos(angle_x);
    float s_x = sin(angle_x);

    float c_y = cos(angle_y);
    float s_y = sin(angle_y);

    float c_x2 = cos(angle_x2);
    float s_x2 = sin(angle_x2);

    rotation_x[I_00] = 1.0f; rotation_x[I_01] = 0;    rotation_x[I_02] = 0;
    rotation_x[I_10] = 0.0f; rotation_x[I_11] = c_x;  rotation_x[I_12] = -s_x;
    rotation_x[I_20] = 0.0f; rotation_x[I_21] = s_x;  rotation_x[I_22] = c_x;

    rotation_y[I_00] = c_y;  rotation_y[I_01] = 0;    rotation_y[I_02] = -s_y;
    rotation_y[I_10] = 0.0f; rotation_y[I_11] = 1;    rotation_y[I_12] = 0;
    rotation_y[I_20] = s_y;  rotation_y[I_21] = 0;    rotation_y[I_22] = c_y;

    rotation_x2[I_00] = 1.0f; rotation_x2[I_01] = 0;    rotation_x2[I_02] = 0;
    rotation_x2[I_10] = 0.0f; rotation_x2[I_11] = c_x2;  rotation_x2[I_12] = -s_x2;
    rotation_x2[I_20] = 0.0f; rotation_x2[I_21] = s_x2;  rotation_x2[I_22] = c_x2;

    float rotation_yx[9];

    multiplyMatrices(rotation_y, rotation_x, rotation_yx);
    multiplyMatrices(rotation_x2, rotation_yx, output);
}

kernel void set_body_rotation(PSO_ARGS, uint body, float3 euler_angles) {
    USE_FIELD_FIRST_VALUE(n, uint) USE_FIELD_FIRST_VALUE(body_count, uint)

    USE_FIELD(body_number, uint)
    USE_FIELD(body_position, float)     USE_FIELD(body_euler_rotation, float)

    USE_FIELD(position, float)          USE_FIELD(rotation, float)

    uint i = get_global_id(0);

    if (i >= n || body_number[i] != body+1 || body >= body_count) {
        barrier(CLK_GLOBAL_MEM_FENCE);

        if (i == 0) vstore3(euler_angles, body, body_euler_rotation);
        return;
    }

    global float * p_rot = rotation + i*3*3;
    float body_rot_inv[9];

    get_matrix_from_euler(vload3(body, body_euler_rotation), body_rot_inv);

    float temp[9];
    float temp2[9];

    transpose(body_rot_inv, temp);

    multiplyMatrices(temp, p_rot, temp2);

    float3 pos = vload3(i, position);

    float3 rpos = (float3)(
        temp2[I_00]*pos.x + temp2[I_01]*pos.y + temp2[I_02]*pos.z,
        temp2[I_10]*pos.x + temp2[I_11]*pos.y + temp2[I_12]*pos.z,
        temp2[I_20]*pos.x + temp2[I_21]*pos.y + temp2[I_22]*pos.z
    );


    multiplyMatrices(temp2, p_rot, temp);

    for (uint i = 0; i < 9; ++i) p_rot[i] = temp[i];

    vstore3(rpos, i, position);

    // TODO centre on body not origin
    // Rotate particles, store both new position and rotation matrix because rotation won't be computed the normal way

    barrier(CLK_GLOBAL_MEM_FENCE);

    if (i == 0) vstore3(euler_angles, body, body_euler_rotation);
}
