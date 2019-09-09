#include <string.h>
#include "frontend.h"

// V0, V1, ..., V15
#define V0_LOCATION "[rsp + 0]"
// TMP
#define TMP_LOCATION "[rsp + 512]"
// S0, S1, ..., S15, VCC, SCC, EXEC
#define S0_LOCATION "[rsp + 512]"
// padding, TMP
//#define TMP_LOCATION "[rsp + 608]"

static const char *kAsmStart = "\
[SECTION .text]\n\
global _start\n\
_start:\n\
    push rbp\n\
    mov rbp, rsp\n\
    and rsp, 0xffffffffffffffe0\n\
    sub rsp, 1024\n\
    mov dword " TMP_LOCATION ", 0xffffffff\n\
    vbroadcastss ymm0, " TMP_LOCATION "\n\
    vmovaps [rsp + 586], ymm0\n\
";

static const char* kAsmEnd = "\
    mov rsp, rbp\n\
    pop rbp\n\
    ret\n\
";


void EmitComment(std::string& str, const Instruction& instr, uint32_t instrIdx, const std::vector<Label>& labels)
{
    char buf[256];

    auto emitOperand = [&](Instruction::Operand op)
    {
        if(op.type == kOperandImmediate)
            sprintf(buf, "0x%x", op.imm);
        else if(op.type == kOperandRegister && op.reg.type == Register::kVector)
            sprintf(buf, "v%u", op.reg.idx);
        else if(op.type == kOperandRegister && op.reg.type == Register::kScalar)
            sprintf(buf, "s%u", op.reg.idx);
        else if(op.type == kOperandRegister && op.reg.type == Register::kEXEC)
            sprintf(buf, "exec");
        else if(op.type == kOperandRegister && op.reg.type == Register::kVCC)
            sprintf(buf, "vcc");
        else if(op.type == kOperandLabelId)
            sprintf(buf, "%s", op.labelId);
        
        str.append(buf);
    };

    for(auto& label : labels)
    {
        if(label.instructionIdx == instrIdx)
        {
            sprintf(buf, "    ; %s:\n", label.name.c_str());
            str.append(buf);
        }
    }

    str.append("    ; ");
    str.append(kOpToMnemonic[instr.opCode]);
    str.append(" ");
    uint32_t opsNum = kOpOperandsNum[instr.opCode];
    for(uint32_t i = 0; i < opsNum; i++)
    {
        emitOperand(instr.operands[i]);
        if (i != opsNum - 1)
            str.append(", ");
    }
    str.append("\n");
}


const char* EmitImmediate(char* buf, uint32_t imm)
{
    sprintf(buf, "0x%x", imm);
    return buf;
}


const char* EmitRegister(char* buf, const Register& reg)
{
    if(reg.type == Register::kVector)
        sprintf(buf, "ymm%u", reg.idx);
    else if(reg.type == Register::kScalar)
        sprintf(buf, "r%ud", 8 + reg.idx);
    else if(reg.type == Register::kVCC)
        sprintf(buf, "ebx");
    else if(reg.type == Register::kEXEC)
        sprintf(buf, "ecx");
    return buf;
}


const char* EmitOperand(char* buf, const Instruction::Operand& op)
{
    if(op.type == kOperandImmediate)
        return EmitImmediate(buf, op.imm);
    else if(op.type == kOperandRegister)
        return EmitRegister(buf, op.reg);
    return nullptr;
}


void EmitInstruction(std::string& str, const char* mnemonic, const Instruction::Operand* operands, uint32_t operandsNum)
{
    char buf[256];
    str.append("    ");
    str.append(mnemonic);
    str.append(" ");
    for(uint32_t i = 0; i < operandsNum; i++)
    {
        str.append(EmitOperand(buf, operands[i]));

        if(i < operandsNum - 1)
            str.append(", ");
    }
    str.append("\n");
}


void GenerateCode(const ParsedCode& parsedCode)
{
    std::string asmbl = kAsmStart;    

    auto addLine = [&](const char* formatStr, ...)
    {
        char buf[512];
        va_list args;
        va_start(args, formatStr);
        vsprintf(buf, formatStr, args);
        va_end(args);

        asmbl.append("    ");
        asmbl.append(buf);
    };

    char buf0[256], buf1[256], buf2[256];

    for(uint32_t i = 0; i < parsedCode.instructions.size(); i++)
    {
        auto& inst = parsedCode.instructions[i];
        EmitComment(asmbl, inst, i, parsedCode.labels);

        switch (inst.opCode)
        {
            case kVectorMov:
            {
                auto& dstReg = inst.operands[0].reg;
                if(inst.operands[1].type == kOperandImmediate)
                {
                    addLine("mov dword " TMP_LOCATION ", 0x%x\n", inst.operands[1].imm);
                    addLine("vbroadcastss ymm%u, " TMP_LOCATION "\n", dstReg.idx);
                }
                else if(inst.operands[1].type == kOperandRegister)
                {
                    auto& srcReg = inst.operands[1].reg;
                    if(srcReg.type == Register::kVector)
                    {
                        EmitInstruction(asmbl, "vmovaps", inst.operands, 2);
                    }
                    else
                    {
                        addLine("mov dword " TMP_LOCATION ", %s\n", EmitRegister(buf1, srcReg));
                        addLine("vbroadcastss ymm%u, " TMP_LOCATION "\n", dstReg.idx);
                    }
                }
                break;
            }

            case kVectorAdd_f32:
                EmitInstruction(asmbl, "vaddps", inst.operands, 3);
                break;

            case kVectorSub_f32:
                EmitInstruction(asmbl, "vsubps", inst.operands, 3);
                break;

            case kVectorMul_f32:
                EmitInstruction(asmbl, "vmulps", inst.operands, 3);
                break;

            case kVectorDiv_f32:
                EmitInstruction(asmbl, "vdivps", inst.operands, 3);
                break;

            case kVectorCmpEq_f32:
            {
                addLine("vmovaps [rsp + 0 * 32], ymm0\n");
                auto& reg0 = inst.operands[0].reg;
                auto& reg1 = inst.operands[1].reg;
                addLine("vcmpeqps ymm0, ymm%u, ymm%u\n", reg0.idx, reg1.idx);
                addLine("vmovmskps ebx, ymm0\n");
                addLine("vmovaps ymm0, [rsp + 0 * 32]\n");
                break;
            }

            case kScalarMov:
                EmitInstruction(asmbl, "mov", inst.operands, 2);
                break;

            case kScalarAnd:
                addLine("mov eax, %s\n", EmitOperand(buf0, inst.operands[1]));
                addLine("and eax, %s\n", EmitOperand(buf0, inst.operands[2]));
                addLine("mov %s, eax\n", EmitOperand(buf0, inst.operands[0]));
                break;

            case kScalarAndN2:
                addLine("mov eax, %s\n", EmitOperand(buf0, inst.operands[2]));
                addLine("andn eax, %s\n", EmitOperand(buf0, inst.operands[1]));
                addLine("mov %s, eax\n", EmitOperand(buf0, inst.operands[0]));
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

    GenerateCode(parsedCode);

    return 0;
}
