grammar VectorAssembler;

start: line+
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
    |   v_cmp_eq_f32
    |   s_mov
    |   s_and
    |   s_andn2
    |   s_branch_vccz
    |   s_branch_vccnz
    |   s_branch_execz
    |   s_branch_execnz
    ;

v_mov:  V_MOV op0=VREGISTER ',' op1=(VREGISTER|SREGISTER|DECIMAL|HEXADECIMAL|FLOAT)
    ;

v_add_f32: V_ADD_F32 op0=VREGISTER ',' op1=VREGISTER ',' op2=VREGISTER
    ;

v_sub_f32: V_SUB_F32 op0=VREGISTER ',' op1=VREGISTER ',' op2=VREGISTER
    ;

v_mul_f32: V_MUL_F32 op0=VREGISTER ',' op1=VREGISTER ',' op2=VREGISTER
    ;

v_div_f32: V_DIV_F32 op0=VREGISTER ',' op1=VREGISTER ',' op2=VREGISTER
    ;

v_cmp_eq_f32: V_CMP_EQ_F32 op0=VREGISTER ',' op1=VREGISTER
    ;

s_mov:  S_MOV op0=SREGISTER ',' op1=(SREGISTER|EXEC|VCC|SCC|DECIMAL|HEXADECIMAL|FLOAT)
    |   S_MOV op0=EXEC ',' op1=(SREGISTER|EXEC|VCC|SCC)
    ;

s_and:  S_AND op0=(SREGISTER|EXEC) ',' op1=(SREGISTER|EXEC|VCC|SCC|DECIMAL|HEXADECIMAL) ',' op2=(SREGISTER|EXEC|VCC|SCC|DECIMAL|HEXADECIMAL)
    ;

s_andn2: S_ANDN2 op0=(SREGISTER|EXEC) ',' op1=(SREGISTER|EXEC|VCC|SCC|DECIMAL|HEXADECIMAL) ',' op2=(SREGISTER|EXEC|VCC|SCC|DECIMAL|HEXADECIMAL)
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
V_CMP_EQ_F32: 'v_cmp_eq_f32';
S_MOV: 's_mov';
S_AND: 's_and';
S_ANDN2: 's_andn2';
S_BRANCH_VCCZ: 's_branch_vccz';
S_BRANCH_VCCNZ: 's_branch_vccnz';
S_BRANCH_EXECZ: 's_branch_execz';
S_BRANCH_EXECNZ: 's_branch_execnz';

VREGISTER: 'v0'|'v1'|'v2'|'v3'|'v4'|'v5'|'v6'|'v7'|'v8'|'v9'|'v10'|'v11'|'v12'|'v13'|'v14'|'v15';
SREGISTER: 's0'|'s1'|'s2'|'s3'|'s4'|'s5'|'s6'|'s7';
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