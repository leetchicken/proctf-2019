v_add_f32 v1, v1, v0 
v_sub_f32 v2, v2, v0

v_mov v0, 1.2
v_mov v1, v5
v_mov v1, s5
s_mov s0, 0x453
# test comment
v_cmp_eq_f32 v1, v0
s_mov s0, exec

s_and s1, 0xff, s2
s_and s1, s1, 2341

label0:
s_and exec, vcc, exec
#s_branch_vccz label1
v_sub_f32 v2, v0, v1
v_mul_f32 v2, v2, v0

label1:
s_andn2 exec, s0, exec
#s_branch_execz label2
v_sub_f32 v2, v1, v0
v_mul_f32 v2, v2, v1

label2:
s_mov exec, s0

s_add s0, s1, s2
s_xor s0, s1, s2
s_shl s0, s1, 2
s_shl s2, 3, 3
s_sub s4, s4, s2
s_sub s4, s2, s4
s_sub s4, 3, s4
s_sub s4, 3, s5
s_add s4, 3, s5
s_add s4, s2, s4
s_add s4q, s4q, s2q
