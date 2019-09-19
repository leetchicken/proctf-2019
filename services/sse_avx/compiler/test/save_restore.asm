[8,1]
v_mov v2, 0

s_mov exec, 0x01
v_mov v2, 1

s_mov exec, 0x02
v_mov v2, 2

s_mov exec, 0x04
v_mov v2, 4

s_mov exec, 0x08
v_mov v2, 8

s_mov exec, 0x10
v_mov v2, 16

s_mov exec, 0x20
v_mov v2, 32

s_mov exec, 0x40
v_mov v2, 64

s_mov exec, 0x80
v_mov v2, 128

s_mov exec, 0xff
v_mul_u32 v1, v1, 8
v_add_u32 v1, v1, v0
v_store v2, v1, s0q
