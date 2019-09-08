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
    kVectorAdd_f32,
    kVectorSub_f32,
    kVectorMul_f32,
    kVectorDiv_f32,
    kVectorCmpEq_f32,

    kScalarMov,
    kScalarAnd,
    kScalarAndN2,
    kScalarBranchVCCZ,
    kScalarBranchVCCNZ,
    kScalarBranchEXECZ,
    kScalarBranchEXECNZ,

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
    kOperandImmediate = 0,
    kOperandRegister = 1,
    kOperandLabelId = 2
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
            const char* labelId;
        };
    };
    Operand operands[3];
};


struct Label
{
    std::string name;
    uint32_t instructionIdx;
};


struct ParsedCode
{
    std::vector<Instruction> instructions;
    std::set<std::string> labelsVisit;
    std::vector<Label> labels;
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
    void enterLabel(VectorAssemblerParser::LabelContext* ctx) override 
    {
        Label label;
        label.name = ctx->id->getText();
        label.instructionIdx = m_parsedCode.instructions.size();
        if(m_parsedCode.labelsVisit.find(label.name) != m_parsedCode.labelsVisit.end())
        {
            auto token = ctx->getStart();
            printf("error %u:%u: label '%s' is already defined\n", token->getLine(), token->getCharPositionInLine(), label.name.c_str());
            m_error = true;
        }
        else
        {
            m_parsedCode.labels.push_back(label);
            m_parsedCode.labelsVisit.insert(label.name);
        }
    }

    void enterV_mov(VectorAssemblerParser::V_movContext* ctx) override
    {
        Instruction instr;
        instr.opCode = kVectorMov;
        instr.type = kInstructionTypeVector;
        instr.operands[0] = ParseOperand(ctx->op0);
        instr.operands[1] = ParseOperand(ctx->op1);
        m_parsedCode.instructions.push_back(instr);
    }

    void enterV_add_f32(VectorAssemblerParser::V_add_f32Context* ctx) override
    {
        Instruction instr;
        instr.opCode = kVectorAdd_f32;
        instr.type = kInstructionTypeVector;
        instr.operands[0] = ParseOperand(ctx->op0);
        instr.operands[1] = ParseOperand(ctx->op1);
        instr.operands[2] = ParseOperand(ctx->op2);
        m_parsedCode.instructions.push_back(instr);
    }

    void enterV_sub_f32(VectorAssemblerParser::V_sub_f32Context* ctx) override
    {
        Instruction instr;
        instr.opCode = kVectorSub_f32;
        instr.type = kInstructionTypeVector;
        instr.operands[0] = ParseOperand(ctx->op0);
        instr.operands[1] = ParseOperand(ctx->op1);
        instr.operands[2] = ParseOperand(ctx->op2);
        m_parsedCode.instructions.push_back(instr);
    }

    void enterV_mul_f32(VectorAssemblerParser::V_mul_f32Context* ctx) override
    {
        Instruction instr;
        instr.opCode = kVectorMul_f32;
        instr.type = kInstructionTypeVector;
        instr.operands[0] = ParseOperand(ctx->op0);
        instr.operands[1] = ParseOperand(ctx->op1);
        instr.operands[2] = ParseOperand(ctx->op2);
        m_parsedCode.instructions.push_back(instr);
    }

    void enterV_div_f32(VectorAssemblerParser::V_div_f32Context* ctx) override
    {
        Instruction instr;
        instr.opCode = kVectorDiv_f32;
        instr.type = kInstructionTypeVector;
        instr.operands[0] = ParseOperand(ctx->op0);
        instr.operands[1] = ParseOperand(ctx->op1);
        instr.operands[2] = ParseOperand(ctx->op2);
        m_parsedCode.instructions.push_back(instr);
    }

    void enterV_cmp_eq_f32(VectorAssemblerParser::V_cmp_eq_f32Context* ctx) override 
    {
        Instruction instr;
        instr.opCode = kVectorCmpEq_f32;
        instr.type = kInstructionTypeVector;
        instr.operands[0] = ParseOperand(ctx->op0);
        instr.operands[1] = ParseOperand(ctx->op1);
        m_parsedCode.instructions.push_back(instr);
    }

    void enterS_mov(VectorAssemblerParser::S_movContext* ctx) override 
    {
        Instruction instr;
        instr.opCode = kScalarMov;
        instr.type = kInstructionTypeScalar;
        instr.operands[0] = ParseOperand(ctx->op0);
        instr.operands[1] = ParseOperand(ctx->op1);
        m_parsedCode.instructions.push_back(instr);
    }

    void enterS_and(VectorAssemblerParser::S_andContext *ctx) override
    {
        Instruction instr;
        instr.opCode = kScalarAnd;
        instr.type = kInstructionTypeScalar;
        instr.operands[0] = ParseOperand(ctx->op0);
        instr.operands[1] = ParseOperand(ctx->op1);
        instr.operands[2] = ParseOperand(ctx->op2);
        m_parsedCode.instructions.push_back(instr);
    }

    void enterS_andn2(VectorAssemblerParser::S_andn2Context* ctx) override
    {
        Instruction instr;
        instr.opCode = kScalarAndN2;
        instr.type = kInstructionTypeScalar;
        instr.operands[0] = ParseOperand(ctx->op0);
        instr.operands[1] = ParseOperand(ctx->op1);
        instr.operands[2] = ParseOperand(ctx->op2);
        m_parsedCode.instructions.push_back(instr);
    }

    void enterS_branch_vccz(VectorAssemblerParser::S_branch_vcczContext* ctx) override 
    {
        Instruction instr;
        instr.opCode = kScalarBranchVCCZ;
        instr.type = kInstructionTypeScalar;
        instr.operands[0] = ParseBranch(ctx->label_id);
        m_parsedCode.instructions.push_back(instr);
    }

    void enterS_branch_vccnz(VectorAssemblerParser::S_branch_vccnzContext* ctx) override 
    {
        Instruction instr;
        instr.opCode = kScalarBranchVCCNZ;
        instr.type = kInstructionTypeScalar;
        instr.operands[0] = ParseBranch(ctx->label_id);
        m_parsedCode.instructions.push_back(instr);
    }

    void enterS_branch_execz(VectorAssemblerParser::S_branch_execzContext* ctx) override 
    {
        Instruction instr;
        instr.opCode = kScalarBranchEXECZ;
        instr.type = kInstructionTypeScalar;
        instr.operands[0] = ParseBranch(ctx->label_id);
        m_parsedCode.instructions.push_back(instr);
    }

    void enterS_branch_execnz(VectorAssemblerParser::S_branch_execnzContext* ctx) override 
    {
        Instruction instr;
        instr.opCode = kScalarBranchEXECNZ;
        instr.type = kInstructionTypeScalar;
        instr.operands[0] = ParseBranch(ctx->label_id);
        m_parsedCode.instructions.push_back(instr);
    }

    const ParsedCode& GetParsedCode() const
    {
        return m_parsedCode;
    }

    bool HasError() const
    {
        return m_error;
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

    Instruction::Operand ParseBranch(antlr4::Token* token)
    {
        auto str = token->getText();
        char* labelId = new char[str.length() + 1];
        memset(labelId, 0, str.length() + 1);
        strcpy(labelId, str.c_str());

        Instruction::Operand op;
        op.type = kOperandLabelId;
        op.labelId = labelId;
        return op;
    }

    ParsedCode m_parsedCode;
    bool m_error = false;
};


class ErrorListener: public antlr4::BaseErrorListener 
{
public:
    void syntaxError(antlr4::Recognizer *recognizer, antlr4::Token *offendingSymbol,
                    size_t line, size_t charPositionInLine, const std::string &msg,
                    std::exception_ptr e) override 
    {
        printf("error %u:%u: %s\n", line, charPositionInLine, msg.c_str());
        m_error = true;
    }

    bool m_error = false;
};


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
    std::ifstream stream;
    stream.open(argv[1]);
    antlr4::ANTLRInputStream input(stream);
    VectorAssemblerLexer lexer(&input);
    antlr4::CommonTokenStream tokens(&lexer);
    ErrorListener errorListener;
    VectorAssemblerParser parser(&tokens);
    parser.removeErrorListeners();
    parser.addErrorListener(&errorListener);

    antlr4::tree::ParseTree *tree = parser.start();
    FrontEnd frontEnd;
    antlr4::tree::ParseTreeWalker::DEFAULT.walk(&frontEnd, tree);

    if(errorListener.m_error || frontEnd.HasError())
        return -1;

    DumpParsedCode(frontEnd.GetParsedCode());
    //printf("\n");
    //GenerateCode(frontEnd.GetParsedCode());

    return 0;
}
