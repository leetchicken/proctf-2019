grammar VectorAssembler;

start: numthreads line+
    |
    ;

numthreads: '[' x=DECIMAL ',' y=DECIMAL ']' NEW_LINE
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
    |   v_cmp_eq_u32
    |   v_cmp_gt_u32
    |   v_cvt_u32_f32
    |   v_cvt_f32_u32
    |   v_load
    |   v_store
    |   s_mov
    |   s_addu
    |   s_and
    |   s_andn2
    |   s_or
    |   s_shl
    |   s_shr
    |   s_load
    |   s_store
    |   s_branch_vccz
    |   s_branch_vccnz
    |   s_branch_execz
    |   s_branch_execnz
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

s_addu: S_ADDU op0=(SREGISTER|EXEC) ',' op1=(SREGISTER|EXEC|VCC|SCC|DECIMAL|HEXADECIMAL) ',' op2=(SREGISTER|EXEC|VCC|SCC|DECIMAL|HEXADECIMAL)
    |   S_ADDU op0=SREGISTER64 ',' op1=(SREGISTER64|DECIMAL|HEXADECIMAL) ',' op2=(SREGISTER64|DECIMAL|HEXADECIMAL)
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

s_branch_vccz: S_BRANCH_VCCZ label_id=ID
    ;

s_branch_vccnz: S_BRANCH_VCCNZ label_id=ID
    ;

s_branch_execz: S_BRANCH_EXECZ label_id=ID
    ;

s_branch_execnz: S_BRANCH_EXECNZ label_id=ID
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
V_CMP_EQ_U32: 'v_cmp_eq_u32';
V_CMP_GT_U32: 'v_cmp_gt_u32';
V_CVT_U32_F32: 'v_cvt_u32_f32';
V_CVT_F32_U32: 'v_cvt_f32_u32';
V_LOAD: 'v_load';
V_STORE: 'v_store';
S_MOV: 's_mov';
S_ADDU: 's_addu';
S_AND: 's_and';
S_ANDN2: 's_andn2';
S_OR: 's_or';
S_SHL: 's_shl';
S_SHR: 's_shr';
S_LOAD: 's_load';
S_STORE: 's_store';
S_BRANCH_VCCZ: 's_branch_vccz';
S_BRANCH_VCCNZ: 's_branch_vccnz';
S_BRANCH_EXECZ: 's_branch_execz';
S_BRANCH_EXECNZ: 's_branch_execnz';

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