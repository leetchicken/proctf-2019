#include <iostream>
#include <string.h>

#include "antlr4-runtime/include/antlr4-runtime.h"
#include "VectorAssemblerLexer.h"
#include "VectorAssemblerParser.h"
#include "VectorAssemblerBaseListener.h"

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


enum EOpCode
{
    kInvalid = 0,

    kVectorMov,
    kVectorCmpEq_f32,

    kScalarMov,

    kOpCodesNum
};


struct KernelHeader
{
    uint32_t vgprNum;
    uint32_t sgrpNum;
};


struct Register
{
    enum Type : uint16_t
    {
        kVector = 0,
        kScalar,
        kVCC,
        kEXEC
    };
    Type type;
    uint16_t idx;
};
static_assert(sizeof(Register) == 4, "");


enum InstructionType
{
    kInstructionTypeVector = 0,
    kInstructionTypeScalar = 1
};


enum OperandType
{
    kOperandImmediate = 1 << 0,
    kOperandRegister = 1 << 1,
};


struct Instruction
{
    InstructionType type;
    EOpCode opCode;

    struct Operand
    {
        OperandType type;
        union
        {
            uint32_t imm;
            Register reg;
        };
    };
    Operand operands[3];
};


Register ParseRegister(const std::string& str)
{
    Register reg;
    char regType = str[0];

    reg.type = regType == 'v' ? Register::kVector : Register::kScalar;
    const char* idxStr = str.data() + 1;
	reg.idx = strtoul(idxStr, nullptr, 10);
    return reg;
}


class FrontEnd : public VectorAssemblerBaseListener 
{
public:
    void enterV_mov(VectorAssemblerParser::V_movContext* ctx) override
    {
        Instruction instr;
        instr.opCode = kVectorMov;
        instr.type = kInstructionTypeVector;
        instr.operands[0] = ParseOperand(ctx->op0);
        instr.operands[1] = ParseOperand(ctx->op1);
        m_instructions.push_back(instr);
    }

    void enterV_cmp_eq_f32(VectorAssemblerParser::V_cmp_eq_f32Context* ctx) override 
    {
        Instruction instr;
        instr.opCode = kVectorCmpEq_f32;
        instr.type = kInstructionTypeVector;
        instr.operands[0] = ParseOperand(ctx->op0);
        instr.operands[1] = ParseOperand(ctx->op1);
        m_instructions.push_back(instr);
    }

    void enterS_mov(VectorAssemblerParser::S_movContext* ctx) override 
    {
        Instruction instr;
        instr.opCode = kScalarMov;
        instr.type = kInstructionTypeScalar;
        instr.operands[0] = ParseOperand(ctx->op0);
        instr.operands[1] = ParseOperand(ctx->op1);
        m_instructions.push_back(instr);
    }

    const std::vector<Instruction>& GetInstructions() const
    {
        return m_instructions;
    }

private:

    Instruction::Operand ParseOperand(antlr4::Token* token)
    {
        Instruction::Operand op;

        auto type = token->getType();
        if(type == VectorAssemblerParser::VREGISTER || type == VectorAssemblerParser::SREGISTER)
        {
            op.type = kOperandRegister;
			op.reg = ParseRegister(token->getText());
        }
        else if(type == VectorAssemblerParser::EXEC)
        {
            op.type = kOperandRegister;
            op.reg.type = Register::kEXEC;
        }
        else if(type == VectorAssemblerParser::VCC)
        {
            op.type = kOperandRegister;
            op.reg.type = Register::kVCC;
        }
        else if(type == VectorAssemblerParser::DECIMAL)
        {
            op.type = kOperandImmediate;
            auto str = token->getText();
            op.imm = strtoul(str.c_str(), nullptr, 10);
        }
        else if(type == VectorAssemblerParser::HEXADECIMAL)
        {
            op.type = kOperandImmediate;
            auto str = token->getText();
            op.imm = strtoul(str.c_str(), nullptr, 16);
        }
        else if(type == VectorAssemblerParser::FLOAT)
        {
            op.type = kOperandImmediate;
            auto str = token->getText();
            float f = strtof(str.c_str(), nullptr);
            memcpy(&op.imm, &f, sizeof(float));
        }

		return op;
    }

    std::vector<Instruction> m_instructions;
};


void GenerateCode(const std::vector<Instruction>& instructions)
{
    std::string asmbl = kAsmStart;    
    char buf[512];

    for(auto& inst : instructions)
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
  std::ifstream stream;
  stream.open(argv[1]);
  antlr4::ANTLRInputStream input(stream);
  VectorAssemblerLexer lexer(&input);
  antlr4::CommonTokenStream tokens(&lexer);
  VectorAssemblerParser parser(&tokens);

  antlr4::tree::ParseTree *tree = parser.start();
  FrontEnd frontEnd;
  antlr4::tree::ParseTreeWalker::DEFAULT.walk(&frontEnd, tree);

  GenerateCode(frontEnd.GetInstructions());

  return 0;
}
