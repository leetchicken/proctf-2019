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
    push rbx\n\
    mov rbp, rsp\n\
    and rsp, 0xffffffffffffffe0\n\
    sub rsp, 32\n\
    mov [rsp], rbp\n\
    sub rsp, 1024\n\
    mov ebx, 0x0\n\
    mov ecx, 0xff\n\
    mov edx, 0xff\n\
";

static const char* kAsmEnd = "\
    add rsp, 1024\n\
    mov rbp, [rsp]\n\
    mov rsp, rbp\n\
    pop rbx\n\
    pop rbp\n\
    ret\n\
";


enum EVecType
{
    kAVX = 0,
    kSSE
};


struct XMM : public Register
{
    XMM(const Register& r)
    {
        type = r.type;
        idx = r.idx;
    }
};


struct YMM : public Register
{
    YMM(const Register& r)
    {
        type = r.type;
        idx = r.idx;
    }
};


void EmitComment(std::string& str, const Instruction& instr)
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

    if(!instr.label.empty())
    {
        sprintf(buf, "    ; %s:\n", instr.label.c_str());
        str.append(buf);
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


template<EVecType type>
const char* EmitRegister(char* buf, const Register& reg)
{
    if(reg.type == Register::kVector)
        sprintf(buf, type == kAVX ? "ymm%u" : "xmm%u", reg.idx);
    else if(reg.type == Register::kScalar)
        sprintf(buf, "r%ud", 8 + reg.idx);
    else if(reg.type == Register::kVCC)
        sprintf(buf, "ebx");
    else if(reg.type == Register::kEXEC)
        sprintf(buf, "ecx");
    return buf;
}


template<EVecType type>
const char* EmitOperand(char* buf, const Instruction::Operand& op)
{
    if(op.type == kOperandImmediate)
        return EmitImmediate(buf, op.imm);
    else if(op.type == kOperandRegister)
        return EmitRegister<type>(buf, op.reg);
    return nullptr;
}


template<EVecType type>
std::string& operator<<(std::string& str, const char* cstr)
{
    str.append(cstr);
    return str;
}


template<EVecType type>
std::string& operator<<(std::string& str, uint32_t imm)
{
    char buf[32];
    str.append(EmitImmediate(buf, imm));
    return str;
}


template<EVecType type>
std::string& operator<<(std::string& str, const Register& reg)
{
    char buf[16];
    str.append(EmitRegister<type>(buf, reg));
    return str;
}


template<EVecType type>
std::string& operator<<(std::string& str, const XMM& xmm)
{
    char buf[16];
    str.append(EmitRegister<kSSE>(buf, xmm));
    return str;
}


template<EVecType type>
std::string& operator<<(std::string& str, const YMM& ymm)
{
    char buf[16];
    str.append(EmitRegister<kAVX>(buf, ymm));
    return str;
}


template<EVecType type>
std::string& operator<<(std::string& str, const Instruction::Operand& op)
{
    char buf[16];
    str.append(EmitOperand<type>(buf, op));
    return str;
}


template<EVecType type>
void AddLine(std::string& str, uint32_t argsNum, uint32_t& argsCounter)
{
    str.append("\n");
}


template<EVecType type, typename T, typename... Args>
void AddLine(std::string& str, uint32_t argsNum, uint32_t& argsCounter, T t, const Args&... args)
{
    operator<<<type>(str, t);
    if(argsCounter == 0 && argsNum > 1)
        str.append(" ");
    else if(argsCounter < argsNum - 1)
        str.append(", ");
    argsCounter++;
    AddLine<type>(str, argsNum, argsCounter, args...);
}


template<EVecType type, bool noTab = false, typename... Args>
void AddLine(std::string& str, const Args&... args)
{
    uint32_t argsNum = sizeof...(args);
    uint32_t argsCounter = 0;
    if(!noTab)
        str.append("    ");
    AddLine<type>(str, argsNum, argsCounter, args...);
}


void AddLine(std::string& str, const char* formatStr, ...)
{
    char buf[512];
    va_list args;
    va_start(args, formatStr);
    vsprintf(buf, formatStr, args);
    va_end(args);

    str.append(buf);
    str.append("\n");
};


template<EVecType type>
void EmitInstruction(std::string& str, const char* mnemonic, const Instruction::Operand* operands, uint32_t operandsNum)
{
    if(type == kAVX || (type == kSSE && operandsNum < 3))
    {
        char buf[256];
        str.append("    ");
        str.append(mnemonic);
        str.append(" ");

        for(uint32_t i = 0; i < operandsNum; i++)
        {
            operator<<<type>(str, EmitOperand<type>(buf, operands[i]));

            if(i < operandsNum - 1)
                str.append(", ");
        }

        str.append("\n");
    }
    else
    {
        if(operands[0].reg != operands[1].reg)
            AddLine<type>(str, "movaps", operands[0], operands[1]);
        AddLine<type>(str, mnemonic, operands[0], operands[2]);
    }
}


void EmitStoreLoad(std::string& asmbl)
{
    const char* storeLoad = "\n\
store_load_avx:\n\
    mov eax, edx\n\
    xor eax, ecx\n\
    jz save_exec_avx\n\
\n\
    mov edx, ecx\n\
    not edx\n\
    and edx, eax\n\
    jz maskload_avx\n\
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
maskload_avx:\n\
    mov edx, ecx\n\
    and edx, eax\n\
    jz save_exec_avx\n\
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
save_exec_avx:\n\
    mov edx, ecx\n\
    jmp rbp\n";

    asmbl.append(storeLoad);
}


template<EVecType type>
void PostScalarInstruction(std::string& asmbl, const Instruction& i)
{
    if(kOpToType[i.opCode] != kInstructionTypeScalar || kOpOperandsNum[i.opCode] == 0)
        return;

    const Instruction::Operand& op0 = i.operands[0];
    if(op0.type != kOperandRegister || op0.reg.type != Register::kEXEC)
        return;

    static uint32_t postScalarCounter = 0;
    const char* postfix = type == kAVX ? "avx" : "sse";

    AddLine(asmbl, "post_scalar_%s_%u_0:", postfix, postScalarCounter);
    AddLine(asmbl, "    lea rbp, [rel $]");
    AddLine(asmbl, "    add rbp, post_scalar_%s_%u_1 - post_scalar_%s_%u_0", postfix, postScalarCounter, postfix, postScalarCounter);
    AddLine(asmbl, "    jmp store_load_avx");
    AddLine(asmbl, "post_scalar_%s_%u_1:", postfix, postScalarCounter);

    AddLine(asmbl, "    mov eax, ecx");
    AddLine(asmbl, "    and eax, 0xf0");
    if(type == kAVX)
        AddLine(asmbl, "    jz post_scalar_sse_%u_2", postScalarCounter);
    else
        AddLine(asmbl, "    jnz post_scalar_avx_%u_2", postScalarCounter);

    AddLine(asmbl, "post_scalar_%s_%u_2:", postfix, postScalarCounter);

    postScalarCounter++;
}


template<EVecType type>
void GenerateCode(std::string& asmbl, const ParsedCode& parsedCode)
{
    for(uint32_t i = 0; i < parsedCode.instructions.size(); i++)
    {
        auto& inst = parsedCode.instructions[i];
        EmitComment(asmbl, inst);

        if(!inst.label.empty())
            AddLine(asmbl, "%s_%s:", inst.label.c_str(), type == kAVX ? "avx" : "sse");

        switch (inst.opCode)
        {
            case kVectorMov:
            {
                auto& dstReg = inst.operands[0].reg;
                auto& srcReg = inst.operands[1].reg;
                if(inst.operands[1].type == kOperandRegister && srcReg.type == Register::kVector)
                {
                    const char* mnemonic = type == kAVX ? "vmovaps" : "movaps";
                    EmitInstruction<type>(asmbl, mnemonic, inst.operands, 2);
                }
                else
                {
                    const char* mnemonic = type == kAVX ? "vmovd" : "movd";
                    if(inst.operands[1].type == kOperandImmediate)
                    {
                        AddLine<type>(asmbl, "mov", "eax", inst.operands[1].imm);
                        AddLine<type>(asmbl, mnemonic, XMM(dstReg), "eax");
                    }
                    else
                    {
                        AddLine<type>(asmbl, mnemonic, XMM(dstReg), srcReg);
                    }
                    
                    if(type == kAVX)
                    {
                        AddLine<type>(asmbl, "vshufps", dstReg, dstReg, dstReg, 0);
                        AddLine<type>(asmbl, "vperm2f128", dstReg, dstReg, dstReg, 0);
                    }
                    else
                    {
                        AddLine<type>(asmbl, "shufps", dstReg, dstReg, 0);
                    }
                }
                break;
            }

            case kVectorAdd_f32:
            {
                const char* mnemonic = type == kAVX ? "vaddps" : "addps";
                EmitInstruction<type>(asmbl, mnemonic, inst.operands, 3);
                break;
            }

            case kVectorSub_f32:
            {
                const char* mnemonic = type == kAVX ? "vsubps" : "subps";
                EmitInstruction<type>(asmbl, mnemonic, inst.operands, 3);
                break;
            }

            case kVectorMul_f32:
            {
                const char* mnemonic = type == kAVX ? "vmulps" : "mulps";
                EmitInstruction<type>(asmbl, mnemonic, inst.operands, 3);
                break;
            }

            case kVectorDiv_f32:
            {
                const char* mnemonic = type == kAVX ? "vdivps" : "divps";
                EmitInstruction<type>(asmbl, mnemonic, inst.operands, 3);
                break;
            }

            case kVectorCmpEq_f32:
            {
                auto& reg0 = inst.operands[0].reg;
                auto& reg1 = inst.operands[1].reg;
                if(type == kAVX)
                {
                    AddLine<type>(asmbl, "vcmpeqps", "ymm15", reg0, reg1);
                    AddLine<type>(asmbl, "vmovmskps ebx, ymm15");
                }
                else
                {
                    AddLine<type>(asmbl, "movaps", "xmm15", reg0);
                    AddLine<type>(asmbl, "cmpeqps", "xmm15", reg1);
                    AddLine<type>(asmbl, "movmskps ebx, xmm15");
                }
                break;
            }

            case kScalarMov:
                EmitInstruction<type>(asmbl, "mov", inst.operands, 2);
                break;

            case kScalarAnd:
            {
                auto& dst = inst.operands[0];
                auto& src0 = inst.operands[1];
                auto& src1 = inst.operands[2];
                if(src1.type == kOperandRegister && dst.reg == src1.reg)
                {
                    AddLine<type>(asmbl, "and", dst, src0);
                }
                else if(src0.type == kOperandRegister && dst.reg == src0.reg)
                {
                    AddLine<type>(asmbl, "and", dst, src1);
                }
                else
                {
                    AddLine<type>(asmbl, "mov", dst, src0);
                    AddLine<type>(asmbl, "and", dst, src1);
                }
                break;
            }

            case kScalarAndN2:
            {
                auto& dst = inst.operands[0];
                auto& src0 = inst.operands[1];
                auto& src1 = inst.operands[2];
                if(src1.type == kOperandRegister && dst.reg == src1.reg)
                {
                    AddLine<type>(asmbl, "not", dst);
                    AddLine<type>(asmbl, "and", dst, src0);
                }
                else if(src0.type == kOperandRegister && dst.reg == src0.reg)
                {
                    AddLine<type>(asmbl, "mov", "eax", src1);
                    AddLine<type>(asmbl, "not eax");
                    AddLine<type>(asmbl, "and", dst, "eax");
                }
                else
                {
                    AddLine<type>(asmbl, "mov", "eax", src1);
                    AddLine<type>(asmbl, "not eax");
                    AddLine<type>(asmbl, "mov", dst, src0);
                    AddLine<type>(asmbl, "and", dst, "eax");
                }
                break;
            }
            
            default:
                break;
        }

        PostScalarInstruction<type>(asmbl, inst);
    }

    AddLine<type>(asmbl, "jmp exit");
}


int main(int argc, const char* argv[]) 
{
    ParsedCode parsedCode;
    if(!Parse(argv[1], parsedCode))
        return -1;

    std::string asmbl = kAsmStart;    

    std::vector<uint32_t> usedSgprRegisters;
    uint32_t usedSgprRegistersMask;
    for(uint32_t i = 0; i < parsedCode.instructions.size(); i++)
    {
        auto& inst = parsedCode.instructions[i];
        uint32_t opsNum = kOpOperandsNum[inst.opCode];
        for(uint32_t j = 0; j < opsNum; j++)
        {
            auto& op = inst.operands[j];
            if(op.type != kOperandRegister)
                continue;
            
            if(op.reg.type != Register::kScalar)
                continue;

            uint32_t x86RegIdx = 8 + op.reg.idx;
            if(x86RegIdx < 12)
                continue;

            usedSgprRegistersMask |= 1 << x86RegIdx;
        }
    }
    while(usedSgprRegistersMask)
    {
        uint32_t idx = __builtin_ctz(usedSgprRegistersMask);
        usedSgprRegisters.push_back(idx);
        usedSgprRegistersMask &= ~(1 << idx);
    }
    for(auto& reg : usedSgprRegisters)
        AddLine(asmbl, "    push r%u", reg);

    GenerateCode<kAVX>(asmbl, parsedCode);
    AddLine<kSSE>(asmbl, "; SSE variant");
    GenerateCode<kSSE>(asmbl, parsedCode);

    AddLine(asmbl, "exit:");
    for(auto iter = usedSgprRegisters.rbegin(); iter != usedSgprRegisters.rend(); ++iter)
        AddLine(asmbl, "    pop r%u", *iter);
    asmbl.append(kAsmEnd);
    EmitStoreLoad(asmbl);

    printf("%s", asmbl.c_str());

    return 0;
}
