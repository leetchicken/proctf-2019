v_mul_u32 v0, v0, 12
v_load v[2:5], v0, s0q
v_add_u32 v0, v0, 4
v_load v[6:9], v0, s0q

v_mul_f32 v10, v2, v6
v_mul_f32 v11, v3, v8
v_add_f32 v10, v10, v11 # c00

v_mul_f32 v11, v2, v7
v_mul_f32 v12, v3, v9
v_add_f32 v11, v11, v12 # c01

v_mul_f32 v2, v4, v6
v_mul_f32 v3, v5, v8
v_add_f32 v12, v2, v3 # c10

v_mul_f32 v6, v4, v7
v_mul_f32 v8, v5, v9
v_add_f32 v13, v6, v8 # c11

v_add_u32 v0, v0, 4
v_store v[10:13], v0, s0q

