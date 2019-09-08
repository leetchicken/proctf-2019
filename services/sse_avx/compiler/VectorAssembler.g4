grammar VectorAssembler;

start: line+
    ;

line: instruction NEW_LINE
    | NEW_LINE
    ;

instruction: v_mov 
    |   v_cmp_eq_f32
    |   s_mov
    ;

v_mov:  V_MOV op0=VREGISTER ',' op1=VREGISTER //# VectorSource
    |   V_MOV op0=VREGISTER ',' op1=SREGISTER //# ScalarSource
    |   V_MOV op0=VREGISTER ',' op1=(DECIMAL|HEXADECIMAL|FLOAT) //# ImmSource
    ;

v_cmp_eq_f32: V_CMP_EQ_F32 op0=VREGISTER ',' op1=VREGISTER
    ;

s_mov:  S_MOV op0=SREGISTER ',' op1=SREGISTER
    |   S_MOV op0=SREGISTER ',' op1=EXEC
    |   S_MOV op0=SREGISTER ',' op1=VCC
    |   S_MOV op0=EXEC ',' op1=SREGISTER
    |   S_MOV op0=SREGISTER ',' op1=(DECIMAL|HEXADECIMAL|FLOAT)
    ;

V_MOV: 'v_mov';
V_CMP_EQ_F32: 'v_cmp_eq_f32';
S_MOV: 's_mov';

VREGISTER: 'v'[0-15];
SREGISTER: 's'[0-15];
EXEC: 'exec';
VCC: 'vcc';
DECIMAL : ('-')? ([0-9])+;
FLOAT : ('-')? ([0-9])+ '.' ([0-9])+;
HEXADECIMAL : '0x' ([a-fA-F0-9])+;
NEW_LINE: '\r'?'\n';
WS: [ \t]+ -> skip;