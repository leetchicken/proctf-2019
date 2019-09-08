#include <string.h>
#include "frontend.h"

#define TMP "[rsp]"
#define VCC_LOCATION "[rsp + 0x20]"
#define EXEC_LOCATION "[rsp + 0x40]"
#define V0  "[rsp + 0x60]"

static const char *kAsmStart = "\
[SECTION .text]\n\
global _start\n\
_start:\n\
    push rbp\n\
    mov rbp, rsp\n\
    and rsp, 0xffffffffffffffe0\n\
    sub rsp, 1056\n\
    mov dword " TMP ", 0xffffffff\n\
    vbroadcastss ymm0, " TMP "\n\
    vmovaps " EXEC_LOCATION ", ymm0\n\
";

static const char* kAsmEnd = "\
    mov rsp, rbp\n\
    pop rbp\n\
    ret\n\
";


void DumpParsedCode(const ParsedCode& parsedCode)
{
    auto dumpOperand = [](Instruction::Operand op)
    {
        if(op.type == kOperandImmediate)
            printf("0x%x", op.imm);
        else if(op.type == kOperandRegister && op.reg.type == Register::kVector)
            printf("v%u", op.reg.idx);
        else if(op.type == kOperandRegister && op.reg.type == Register::kScalar)
            printf("s%u", op.reg.idx);
        else if(op.type == kOperandRegister && op.reg.type == Register::kEXEC)
            printf("exec");
        else if(op.type == kOperandRegister && op.reg.type == Register::kVCC)
            printf("vcc");
    };

    auto dumpOperands = [&](const Instruction& instr, uint32_t num)
    {
        for(uint32_t i = 0; i < num; i++)
        {
            dumpOperand(instr.operands[i]);
            if (i != num - 1)
                printf(", ");
        }
        printf("\n");
    };

    for(size_t i = 0; i < parsedCode.instructions.size(); i++)
    {
        auto& inst = parsedCode.instructions[i];
        for(auto& label : parsedCode.labels)
        {
            if(label.instructionIdx == i)
                printf("%s:\n", label.name.c_str());
        }

        switch (inst.opCode)
        {
        case kVectorMov:
            printf("v_mov ");
            dumpOperands(inst, 2);
            break;

        case kVectorAdd_f32:
            printf("v_add_f32 ");
            dumpOperands(inst, 3);
            break;

        case kVectorSub_f32:
            printf("v_sub_f32 ");
            dumpOperands(inst, 3);
            break;

        case kVectorMul_f32:
            printf("v_mul_f32 ");
            dumpOperands(inst, 3);
            break;

        case kVectorDiv_f32:
            printf("v_div_f32 ");
            dumpOperands(inst, 3);
            break;

        case kVectorCmpEq_f32:
            printf("v_cmp_eq_f32 ");
            dumpOperands(inst, 2);
            break;
        
        case kScalarMov:
            printf("s_mov ");
            dumpOperands(inst, 2);
            break;

        case kScalarAnd:
            printf("s_and ");
            dumpOperands(inst, 3);
            break;

        case kScalarAndN2:
            printf("s_andn2 ");
            dumpOperands(inst, 3);
            break;

        case kScalarBranchVCCZ:
            printf("s_branch_vccz %s\n", inst.operands[0].labelId);
            break;

        case kScalarBranchVCCNZ:
            printf("s_branch_vccnz %s\n", inst.operands[0].labelId);
            break;

        case kScalarBranchEXECZ:
            printf("s_branch_execz %s\n", inst.operands[0].labelId);
            break;

        case kScalarBranchEXECNZ:
            printf("s_branch_execnz %s\n", inst.operands[0].labelId);
            break;

        default:
            break;
        }
    }
}


void GenerateCode(const ParsedCode& parsedCode)
{
    std::string asmbl = kAsmStart;    
    char buf[512];

    for(auto& inst : parsedCode.instructions)
    {
        switch (inst.opCode)
        {
        case kVectorMov:
            if(inst.operands[1].type == kOperandImmediate)
            {
                sprintf(buf, "mov dword " TMP ", 0x%x\n", inst.operands[1].imm);
                asmbl.append(buf);
                auto& reg = inst.operands[0].reg;
                sprintf(buf, "vbroadcastss ymm%u, " TMP "\n", reg.idx);
                asmbl.append(buf);
            }
            else if(inst.operands[1].type == kOperandRegister)
            {
                auto& dstReg = inst.operands[0].reg;
                auto& srcReg = inst.operands[1].reg;
                if(srcReg.type == Register::kVector)
                    sprintf(buf, "vmovaps ymm%u, ymm%u\n", dstReg.idx, srcReg.idx);
                if(srcReg.type == Register::kScalar)
                    sprintf(buf, "vbroadcastss ymm%u, [rdi + %u * 4]\n", dstReg.idx, srcReg.idx);
                asmbl.append(buf);
            }
            break;

        case kScalarMov:
            break;

        case kVectorCmpEq_f32:
            {
                sprintf(buf, "vmovaps [rsi + 0 * 32], ymm0\n");
                asmbl.append(buf);
                auto& reg0 = inst.operands[0].reg;
                auto& reg1 = inst.operands[1].reg;
                sprintf(buf, "vcmpeqps ymm0, ymm%u, ymm%u\n", reg0.idx, reg1.idx);
                asmbl.append(buf);
                sprintf(buf, "vmovmskps eax, ymm0\n");
                asmbl.append(buf);
                sprintf(buf, "mov dword [rdi + %u * 4], eax\n", VCC_LOCATION);
                asmbl.append(buf);
                sprintf(buf, "vmovaps ymm0, [rsi + 0 * 32]\n");
                asmbl.append(buf);
            }
            break;
        
        default:
            break;
        }
    }

    asmbl.append(kAsmEnd);

    printf("%s", asmbl.c_str());
}


int main(int argc, const char* argv[]) 
{
    ParsedCode parsedCode;
    if(!Parse(argv[1], parsedCode))
        return -1;

    DumpParsedCode(parsedCode);
    //printf("\n");
    //GenerateCode(frontEnd.GetParsedCode());

    return 0;
}
