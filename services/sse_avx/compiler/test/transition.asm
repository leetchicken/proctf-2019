[8,1]
v_mov v0, 0
v_load v1, 0, s0q # 1, -1
v_mov v2, -1.0

v_mul_f32 v1, v1, v2 # -1, 1
v_cmp_gt_f32 v1, v0
s_mov exec, vcc #0x0f

v_add_f32 v1, v1, v1 # -1, 2
s_mov exec, 0xff 

v_mul_f32 v1, v1, v2 # 1, -2
v_cmp_gt_f32 v1, v0
s_mov exec, vcc #0xf0

v_add_f32 v1, v1, v1 # 2, -2

s_mov exec, 0xff

