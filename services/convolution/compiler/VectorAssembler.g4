grammar VectorAssembler;

start: line+
    |
    ;

line: instruction NEW_LINE
    | label NEW_LINE
    | NEW_LINE
    ;

label: id=ID ':'
    ;

instruction: v_mov 
    |   v_add_f32
    |   v_sub_f32
    |   v_mul_f32
    |   v_div_f32
    |   v_cmp_lt_f32
    |   v_cmp_le_f32
    |   v_cmp_eq_f32
    |   v_cmp_gt_f32
    |   v_cmp_ge_f32
    |   v_cmp_ne_f32
    |   v_add_u32
    |   v_sub_u32
    |   v_mul_u32
    |   v_sll_u32
    |   v_srl_u32
    |   v_and_u32
    |   v_andnot_u32
    |   v_or_u32
    |   v_xor_u32
    |   v_cmp_eq_u32
    |   v_cmp_gt_u32
    |   v_cvt_u32_f32
    |   v_cvt_f32_u32
    |   v_load
    |   v_store
    |   s_mov
    |   s_add
    |   s_sub
    |   s_and
    |   s_andn2
    |   s_or
    |   s_xor
    |   s_shl
    |   s_shr
    |   s_load
    |   s_store
    ;

v_mov:  V_MOV op0=VREGISTER ',' op1=(VREGISTER|SREGISTER|DECIMAL|HEXADECIMAL|FLOAT)
    ;

v_add_f32: V_ADD_F32 op0=VREGISTER ',' op1=VREGISTER ',' op2=(VREGISTER|DECIMAL|FLOAT)
    ;

v_sub_f32: V_SUB_F32 op0=VREGISTER ',' op1=VREGISTER ',' op2=(VREGISTER|DECIMAL|FLOAT)
    ;

v_mul_f32: V_MUL_F32 op0=VREGISTER ',' op1=VREGISTER ',' op2=(VREGISTER|DECIMAL|FLOAT)
    ;

v_div_f32: V_DIV_F32 op0=VREGISTER ',' op1=VREGISTER ',' op2=(VREGISTER|DECIMAL|FLOAT)
    ;

v_cmp_lt_f32: V_CMP_LT_F32 op0=VREGISTER ',' op1=VREGISTER
    ;

v_cmp_le_f32: V_CMP_LE_F32 op0=VREGISTER ',' op1=VREGISTER
    ;

v_cmp_eq_f32: V_CMP_EQ_F32 op0=VREGISTER ',' op1=VREGISTER
    ;

v_cmp_gt_f32: V_CMP_GT_F32 op0=VREGISTER ',' op1=VREGISTER
    ;

v_cmp_ge_f32: V_CMP_GE_F32 op0=VREGISTER ',' op1=VREGISTER
    ;

v_cmp_ne_f32: V_CMP_NE_F32 op0=VREGISTER ',' op1=VREGISTER
    ;

v_add_u32: V_ADD_U32 op0=VREGISTER ',' op1=VREGISTER ',' op2=(VREGISTER|DECIMAL|HEXADECIMAL)
    ;

v_sub_u32: V_SUB_U32 op0=VREGISTER ',' op1=VREGISTER ',' op2=(VREGISTER|DECIMAL|HEXADECIMAL)
    ;

v_mul_u32: V_MUL_U32 op0=VREGISTER ',' op1=VREGISTER ',' op2=(VREGISTER|DECIMAL|HEXADECIMAL)
    ;

v_sll_u32: V_SLL_U32 op0=VREGISTER ',' op1=VREGISTER ',' op2=(VREGISTER|DECIMAL|HEXADECIMAL)
    ;

v_srl_u32: V_SRL_U32 op0=VREGISTER ',' op1=VREGISTER ',' op2=(VREGISTER|DECIMAL|HEXADECIMAL)
    ;

v_and_u32: V_AND_U32 op0=VREGISTER ',' op1=VREGISTER ',' op2=(VREGISTER|DECIMAL|HEXADECIMAL)
    ;

v_andnot_u32: V_ANDNOT_U32 op0=VREGISTER ',' op1=VREGISTER ',' op2=(VREGISTER|DECIMAL|HEXADECIMAL)
    ;

v_or_u32: V_OR_U32 op0=VREGISTER ',' op1=VREGISTER ',' op2=(VREGISTER|DECIMAL|HEXADECIMAL)
    ;

v_xor_u32: V_XOR_U32 op0=VREGISTER ',' op1=VREGISTER ',' op2=(VREGISTER|DECIMAL|HEXADECIMAL)
    ;

v_cmp_eq_u32: V_CMP_EQ_U32 op0=VREGISTER ',' op1=VREGISTER
    ;

v_cmp_gt_u32: V_CMP_GT_U32 op0=VREGISTER ',' op1=VREGISTER
    ;

v_cvt_u32_f32: V_CVT_U32_F32 op0=VREGISTER ',' op1=VREGISTER
    ;

v_cvt_f32_u32: V_CVT_F32_U32 op0=VREGISTER ',' op1=VREGISTER
    ;

v_load: V_LOAD op0=(VREGISTER|VREGISTER_RANGE) ',' op1=(VREGISTER|SREGISTER|DECIMAL|HEXADECIMAL) ',' op2=SREGISTER64
    ;

v_store: V_STORE op0=(VREGISTER|VREGISTER_RANGE) ',' op1=(VREGISTER|SREGISTER|DECIMAL|HEXADECIMAL) ',' op2=SREGISTER64
    ;

s_mov:  S_MOV op0=SREGISTER ',' op1=(SREGISTER|EXEC|VCC|SCC|DECIMAL|HEXADECIMAL|FLOAT)
    |   S_MOV op0=EXEC ',' op1=(SREGISTER|EXEC|VCC|SCC|DECIMAL|HEXADECIMAL)
    |   S_MOV op0=SREGISTER64 ',' op1=(SREGISTER64|DECIMAL|HEXADECIMAL)
    ;

s_add: S_ADD op0=(SREGISTER|EXEC) ',' op1=(SREGISTER|EXEC|VCC|SCC|DECIMAL|HEXADECIMAL) ',' op2=(SREGISTER|EXEC|VCC|SCC|DECIMAL|HEXADECIMAL)
    |  S_ADD op0=SREGISTER64 ',' op1=(SREGISTER64|DECIMAL|HEXADECIMAL) ',' op2=(SREGISTER64|DECIMAL|HEXADECIMAL)
    ;

s_sub: S_SUB op0=(SREGISTER|EXEC) ',' op1=(SREGISTER|EXEC|VCC|SCC|DECIMAL|HEXADECIMAL) ',' op2=(SREGISTER|EXEC|VCC|SCC|DECIMAL|HEXADECIMAL)
    |  S_SUB op0=SREGISTER64 ',' op1=(SREGISTER64|DECIMAL|HEXADECIMAL) ',' op2=(SREGISTER64|DECIMAL|HEXADECIMAL)
    ;

s_and:  S_AND op0=(SREGISTER|EXEC) ',' op1=(SREGISTER|EXEC|VCC|SCC|DECIMAL|HEXADECIMAL) ',' op2=(SREGISTER|EXEC|VCC|SCC|DECIMAL|HEXADECIMAL)
    |   S_AND op0=SREGISTER64 ',' op1=(SREGISTER64|DECIMAL|HEXADECIMAL) ',' op2=(SREGISTER64|DECIMAL|HEXADECIMAL)
    ;

s_andn2: S_ANDN2 op0=(SREGISTER|EXEC) ',' op1=(SREGISTER|EXEC|VCC|SCC|DECIMAL|HEXADECIMAL) ',' op2=(SREGISTER|EXEC|VCC|SCC|DECIMAL|HEXADECIMAL)
    |   S_ANDN2 op0=SREGISTER64 ',' op1=(SREGISTER64|DECIMAL|HEXADECIMAL) ',' op2=(SREGISTER64|DECIMAL|HEXADECIMAL)
    ;

s_or:   S_OR op0=(SREGISTER|EXEC) ',' op1=(SREGISTER|EXEC|VCC|SCC|DECIMAL|HEXADECIMAL) ',' op2=(SREGISTER|EXEC|VCC|SCC|DECIMAL|HEXADECIMAL)
    |   S_OR op0=SREGISTER64 ',' op1=(SREGISTER64|DECIMAL|HEXADECIMAL) ',' op2=(SREGISTER64|DECIMAL|HEXADECIMAL)
    ;

s_xor:  S_XOR op0=(SREGISTER|EXEC) ',' op1=(SREGISTER|EXEC|VCC|SCC|DECIMAL|HEXADECIMAL) ',' op2=(SREGISTER|EXEC|VCC|SCC|DECIMAL|HEXADECIMAL)
    |   S_XOR op0=SREGISTER64 ',' op1=(SREGISTER64|DECIMAL|HEXADECIMAL) ',' op2=(SREGISTER64|DECIMAL|HEXADECIMAL)
    ;

s_shl: S_SHL op0=(SREGISTER|EXEC) ',' op1=(SREGISTER|EXEC|VCC|SCC|DECIMAL|HEXADECIMAL) ',' op2=(DECIMAL|HEXADECIMAL)
    |  S_SHL op0=SREGISTER64 ',' op1=(SREGISTER64|DECIMAL|HEXADECIMAL) ',' op2=(DECIMAL|HEXADECIMAL)
    ;

s_shr: S_SHR op0=(SREGISTER|EXEC) ',' op1=(SREGISTER|EXEC|VCC|SCC|DECIMAL|HEXADECIMAL) ',' op2=(DECIMAL|HEXADECIMAL)
    |  S_SHR op0=SREGISTER64 ',' op1=(SREGISTER64|DECIMAL|HEXADECIMAL) ',' op2=(DECIMAL|HEXADECIMAL)
    ;

s_load: S_LOAD op0=(SREGISTER|SREGISTER64) ',' op1=(SREGISTER|SREGISTER64|DECIMAL|HEXADECIMAL) ',' op2=SREGISTER64
    ;

s_store: S_STORE op0=(SREGISTER|SREGISTER64) ',' op1=(SREGISTER|SREGISTER64|DECIMAL|HEXADECIMAL) ',' op2=SREGISTER64
    ;



V_MOV: 'v_mov';
V_ADD_F32: 'v_add_f32';
V_SUB_F32: 'v_sub_f32';
V_MUL_F32: 'v_mul_f32';
V_DIV_F32: 'v_div_f32';
V_CMP_LT_F32: 'v_cmp_lt_f32';
V_CMP_LE_F32: 'v_cmp_le_f32';
V_CMP_EQ_F32: 'v_cmp_eq_f32';
V_CMP_GT_F32: 'v_cmp_gt_f32';
V_CMP_GE_F32: 'v_cmp_ge_f32';
V_CMP_NE_F32: 'v_cmp_ne_f32';
V_ADD_U32: 'v_add_u32';
V_SUB_U32: 'v_sub_u32';
V_MUL_U32: 'v_mul_u32';
V_SLL_U32: 'v_sll_u32';
V_SRL_U32: 'v_srl_u32';
V_AND_U32: 'v_and_u32';
V_ANDNOT_U32: 'v_andnot_u32';
V_OR_U32: 'v_or_u32';
V_XOR_U32: 'v_xor_u32';
V_CMP_EQ_U32: 'v_cmp_eq_u32';
V_CMP_GT_U32: 'v_cmp_gt_u32';
V_CVT_U32_F32: 'v_cvt_u32_f32';
V_CVT_F32_U32: 'v_cvt_f32_u32';
V_LOAD: 'v_load';
V_STORE: 'v_store';
S_MOV: 's_mov';
S_ADD: 's_add';
S_SUB: 's_sub';
S_AND: 's_and';
S_ANDN2: 's_andn2';
S_OR: 's_or';
S_XOR: 's_xor';
S_SHL: 's_shl';
S_SHR: 's_shr';
S_LOAD: 's_load';
S_STORE: 's_store';

VREGISTER: 'v0'|'v1'|'v2'|'v3'|'v4'|'v5'|'v6'|'v7'|'v8'|'v9'|'v10'|'v11'|'v12'|'v13'|'v14';
VREGISTER_RANGE: 'v[' ([0-9])+ ':' ([0-9])+ ']';
SREGISTER: 's0'|'s1'|'s2'|'s3'|'s4'|'s5'|'s6'|'s7';
SREGISTER64: 's0q'|'s1q'|'s2q'|'s3q'|'s4q'|'s5q'|'s6q'|'s7q';
VCC: 'vcc';
SCC: 'scc';
EXEC: 'exec';
DECIMAL : ('-')? ([0-9])+;
FLOAT : ('-')? ([0-9])+ '.' ([0-9])+;
HEXADECIMAL : '0x' ([a-fA-F0-9])+;
ID: [a-zA-Z] ([0-9a-zA-Z_])*;
NEW_LINE: '\r'?'\n';
WS: [ \t]+ -> skip;
COMMENT : '#' ~[\r\n]* '\r'? '\n' -> skip;