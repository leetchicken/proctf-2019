# source image
s_load s1q, 0, s0q
# width
s_load s3, 8, s0q
# height
s_load s4, 12, s0q

# save indices
v_mov v8, v0
v_mov v9, v1

# load image
v_mov v10, s3
v_mul_u32 v10, v10, 3
v_mul_u32 v10, v1, v10
v_mul_u32 v0, v0, 3
v_add_u32 v10, v0, v10

v_mov v11, v10
v_load v0, v11, s1q

v_add_u32 v11, v11, 1
v_load v1, v11, s1q

v_add_u32 v11, v11, 1
v_load v2, v11, s1q

v_mov v12, s3
v_add_u32 v11, v10, v12
v_load v3, v11, s1q

v_add_u32 v11, v11, 2
v_load v4, v11, s1q

v_mov v12, s3
v_add_u32 v11, v10, v12
v_add_u32 v11, v11, v12
v_load v5, v11, s1q

v_add_u32 v11, v11, 1
v_load v6, v11, s1q

v_add_u32 v11, v11, 1
v_load v7, v11, s1q

# 
v_mov v12, 0
v_mov v13, 0.0

# 0th byte
s_load s1q, 16, s0q
s_load s1q, 0, s1q
s_and s1q, s1q, 0xff
v_mov v11, s1
v_cmp_eq_u32 v0, v11
s_mov s2, vcc
v_cmp_gt_u32 v0, v11
s_or exec, s2, vcc
v_add_u32 v12, v12, v0
v_add_f32 v13, v13, 1.0
s_mov exec, 0xff

# 1st byte
s_load s1q, 16, s0q
s_load s1q, 0, s1q
s_shr s1q, s1q, 8
s_and s1q, s1q, 0xff
v_mov v11, s1
v_cmp_eq_u32 v1, v11
s_mov s2, vcc
v_cmp_gt_u32 v1, v11
s_or exec, s2, vcc
v_add_u32 v12, v12, v1
v_add_f32 v13, v13, 1.0
s_mov exec, 0xff

# 2nd byte
s_load s1q, 16, s0q
s_load s1q, 0, s1q
s_shr s1q, s1q, 16
s_and s1q, s1q, 0xff
v_mov v11, s1
v_cmp_eq_u32 v2, v11
s_mov s2, vcc
v_cmp_gt_u32 v2, v11
s_or exec, s2, vcc
v_add_u32 v12, v12, v2
v_add_f32 v13, v13, 1.0
s_mov exec, 0xff

# 3rd byte
s_load s1q, 16, s0q
s_load s1q, 0, s1q
s_shr s1q, s1q, 24
s_and s1q, s1q, 0xff
v_mov v11, s1
v_cmp_eq_u32 v3, v11
s_mov s2, vcc
v_cmp_gt_u32 v3, v11
s_or exec, s2, vcc
v_add_u32 v12, v12, v3
v_add_f32 v13, v13, 1.0
s_mov exec, 0xff

# 4th byte
s_load s1q, 16, s0q
s_load s1q, 0, s1q
s_shr s1q, s1q, 32
s_and s1q, s1q, 0xff
v_mov v11, s1
v_cmp_eq_u32 v4, v11
s_mov s2, vcc
v_cmp_gt_u32 v4, v11
s_or exec, s2, vcc
v_add_u32 v12, v12, v4
v_add_f32 v13, v13, 1.0
s_mov exec, 0xff

# 5th byte
s_load s1q, 16, s0q
s_load s1q, 0, s1q
s_shr s1q, s1q, 40
s_and s1q, s1q, 0xff
v_mov v11, s1
v_cmp_eq_u32 v5, v11
s_mov s2, vcc
v_cmp_gt_u32 v5, v11
s_or exec, s2, vcc
v_add_u32 v12, v12, v5
v_add_f32 v13, v13, 1.0
s_mov exec, 0xff

# 6th byte
s_load s1q, 16, s0q
s_load s1q, 0, s1q
s_shr s1q, s1q, 48
s_and s1q, s1q, 0xff
v_mov v11, s1
v_cmp_eq_u32 v6, v11
s_mov s2, vcc
v_cmp_gt_u32 v6, v11
s_or exec, s2, vcc
v_add_u32 v12, v12, v6
v_add_f32 v13, v13, 1.0
s_mov exec, 0xff

# 7th byte
s_load s1q, 16, s0q
s_load s1q, 0, s1q
s_shr s1q, s1q, 56
s_and s1q, s1q, 0xff
v_mov v11, s1
v_cmp_eq_u32 v7, v11
s_mov s2, vcc
v_cmp_gt_u32 v7, v11
s_or exec, s2, vcc
v_add_u32 v12, v12, v7
v_add_f32 v13, v13, 1.0
s_mov exec, 0xff

# average
v_cvt_u32_f32 v12, v12
v_mov v0, 0.0
v_cmp_gt_f32 v13, v0
s_mov exec, vcc
v_div_f32 v12, v12, v13
s_mov exec, 0xff
v_cvt_f32_u32 v12, v12

# dest image
s_load s1q, 24, s0q
s_load s2, 32, s0q
s_load s3, 36, s0q

# restore indices
v_mov v0, v8
v_mov v1, v9

v_mov v10, s2
v_mul_u32 v10, v1, v10
v_add_u32 v10, v0, v10

v_store v12, v10, s1q
