v_mov v10, 0.0
# 1, -1
v_load v11, 0, s0q
v_mov v12, -1.0

# -1, 1
v_mul_f32 v11, v11, v12 
v_cmp_gt_f32 v11, v10
s_mov exec, vcc #0x0f

# -1, 2
v_add_f32 v11, v11, v11
s_mov exec, 0xff 

# 1, -2
v_mul_f32 v11, v11, v12
v_cmp_gt_f32 v11, v10
s_mov exec, vcc #0xf0

# 2, -2
v_add_f32 v11, v11, v11

s_mov exec, 0xff
v_mul_u32 v1, v1, 8
v_add_u32 v1, v1, v0
s_add s4q, s0q, 32
v_store v11, v1, s4q
