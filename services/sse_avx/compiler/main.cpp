#include <string.h>
#include "frontend.h"

// V0, V1, ..., V15
#define V0_LOCATION "[rsp + 0]"
// TMP
#define TMP_LOCATION "[rsp + 512]"
// padding, TMP
//#define TMP_LOCATION "[rsp + 608]"

static const char *kAsmStart = "\
[SECTION .text]\n\
global _start\n\
_start:\n\
    push rbp\n\
    mov rbp, rsp\n\
    and rsp, 0xffffffffffffffe0\n\
    sub rsp, 32\n\
    mov dword [rsp], rbp\n\
    sub rsp, 1024\n\
    mov ebx, 0x0\n\
    mov ecx, 0xff\n\
    mov edx, 0xff\n\
";

static const char* kAsmEnd = "\
    add rsp, 1024\n\
    mov rbp, [rsp]\n\
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


void PostScalarInstruction(std::string& asmbl, const Instruction& i)
{
    if(kOpToType[i.opCode] != kInstructionTypeScalar || kOpOperandsNum[i.opCode] == 0)
        return;

    const Instruction::Operand& op0 = i.operands[0];
    if(op0.type != kOperandRegister || op0.reg.type != Register::kEXEC)
        return;

    const char* storeLoad = "\
    mov eax, edx\n\
    xor eax, ecx\n\
    jz save_exec%u\n\
\n\
    mov edx, ecx\n\
    not edx\n\
    and edx, eax\n\
    jz maskload%u\n\
    ; unpack and store\n\
    vmovd xmm15, edx\n\
    vshufps ymm15, ymm15, ymm15, 0\n\
    vperm2f128 ymm15, ymm15, ymm15, 0\n\
    vpand ymm15, ymm15, [rdi]\n\
    vpcmpeqd ymm15, ymm15, [rdi]\n\
    vmaskmovps [rsp], ymm15, ymm0\n\
    vmaskmovps [rsp+32], ymm15, ymm1\n\
    vmaskmovps [rsp+64], ymm15, ymm2\n\
    vmaskmovps [rsp+96], ymm15, ymm3\n\
    vmaskmovps [rsp+128], ymm15, ymm4\n\
    vmaskmovps [rsp+160], ymm15, ymm5\n\
    vmaskmovps [rsp+192], ymm15, ymm6\n\
    vmaskmovps [rsp+224], ymm15, ymm7\n\
    vmaskmovps [rsp+256], ymm15, ymm8\n\
    vmaskmovps [rsp+288], ymm15, ymm9\n\
    vmaskmovps [rsp+320], ymm15, ymm10\n\
    vmaskmovps [rsp+352], ymm15, ymm11\n\
    vmaskmovps [rsp+384], ymm15, ymm12\n\
    vmaskmovps [rsp+416], ymm15, ymm13\n\
    vmaskmovps [rsp+448], ymm15, ymm14\n\
\n\
maskload%u:\n\
    mov edx, ecx\n\
    and edx, eax\n\
    jz save_exec%u\n\
    ; unpack and load\n\
    vmovd xmm15, edx\n\
    vshufps ymm15, ymm15, ymm15, 0\n\
    vperm2f128 ymm15, ymm15, ymm15, 0\n\
    vpand ymm15, ymm15, [rdi]\n\
    vpcmpeqd ymm15, ymm15, [rdi]\n\
    vmaskmovps ymm0, ymm15, [rsp]\n\
    vmaskmovps ymm1, ymm15, [rsp+32]\n\
    vmaskmovps ymm2, ymm15, [rsp+64]\n\
    vmaskmovps ymm3, ymm15, [rsp+96]\n\
    vmaskmovps ymm4, ymm15, [rsp+128]\n\
    vmaskmovps ymm5, ymm15, [rsp+160]\n\
    vmaskmovps ymm6, ymm15, [rsp+192]\n\
    vmaskmovps ymm7, ymm15, [rsp+224]\n\
    vmaskmovps ymm8, ymm15, [rsp+256]\n\
    vmaskmovps ymm9, ymm15, [rsp+288]\n\
    vmaskmovps ymm10, ymm15, [rsp+320]\n\
    vmaskmovps ymm11, ymm15, [rsp+352]\n\
    vmaskmovps ymm12, ymm15, [rsp+384]\n\
    vmaskmovps ymm13, ymm15, [rsp+416]\n\
    vmaskmovps ymm14, ymm15, [rsp+448]\n\
\n\
save_exec%u:\n\
    mov edx, ecx\n";

    static uint32_t storeLoadCounter = 0;
    char buf[2048];
    sprintf(buf, storeLoad, storeLoadCounter, storeLoadCounter, storeLoadCounter, storeLoadCounter, storeLoadCounter);
    storeLoadCounter++;
    asmbl.append(buf);
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
                    addLine("mov eax, 0x%x\n", inst.operands[1].imm);
                    addLine("vmovd xmm%u, eax\n", dstReg.idx);
                    addLine("vshufps ymm%u, ymm, ymm%u, 0\n", dstReg.idx);
                    addLine("vperm2f128 ymm%u, ymm%u, ymm%u, 0\n", dstReg.idx);
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
                        addLine("vmovd xmm%u, %s\n", dstReg.idx, EmitRegister(buf1, srcReg));
                        addLine("vshufps ymm%u, ymm, ymm%u, 0\n", dstReg.idx);
                        addLine("vperm2f128 ymm%u, ymm%u, ymm%u, 0\n", dstReg.idx);
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
                auto& reg0 = inst.operands[0].reg;
                auto& reg1 = inst.operands[1].reg;
                addLine("vcmpeqps ymm15, ymm%u, ymm%u\n", reg0.idx, reg1.idx);
                addLine("vmovmskps ebx, ymm15\n");
                break;
            }

            case kScalarMov:
                EmitInstruction(asmbl, "mov", inst.operands, 2);
                break;

            case kScalarAnd:
            {
                auto& dst = inst.operands[0];
                auto& src0 = inst.operands[1];
                auto& src1 = inst.operands[2];
                if(dst == src1)
                {
                    addLine("and %s, %s\n", EmitOperand(buf0, dst), EmitOperand(buf1, src0));
                }
                else if(dst == src0)
                {
                    addLine("and %s, %s\n", EmitOperand(buf0, dst), EmitOperand(buf1, src1));
                }
                else
                {
                    addLine("mov %s, %s\n", EmitOperand(buf0, dst), EmitOperand(buf1, src0));
                    addLine("and %s, %s\n", EmitOperand(buf0, dst), EmitOperand(buf1, src1));
                }
                break;
            }

            case kScalarAndN2:
            {
                auto& dst = inst.operands[0];
                auto& src0 = inst.operands[1];
                auto& src1 = inst.operands[2];
                if(dst == src1)
                {
                    addLine("not %s\n", EmitOperand(buf0, dst));
                    addLine("and %s, %s\n", EmitOperand(buf0, dst), EmitOperand(buf1, src0));
                }
                else if(dst == src0)
                {
                    addLine("mov eax, %s\n", EmitOperand(buf1, src1));
                    addLine("not eax\n");
                    addLine("and %s, eax\n", EmitOperand(buf0, dst));
                }
                else
                {
                    addLine("mov eax, %s\n", EmitOperand(buf1, src1));
                    addLine("not eax\n");
                    addLine("mov %s, %s\n", EmitOperand(buf0, dst), EmitOperand(buf1, src0));
                    addLine("and %s, eax\n", EmitOperand(buf0, dst));
                }
                break;
            }

            
            default:
                break;
        }

        PostScalarInstruction(asmbl, inst);
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
