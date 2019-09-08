#pragma once
#include "antlr4-runtime/include/antlr4-runtime.h"
#include "VectorAssemblerLexer.h"
#include "VectorAssemblerParser.h"
#include "VectorAssemblerBaseListener.h"


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


class FrontEnd : public VectorAssemblerBaseListener 
{
public:
    void enterLabel(VectorAssemblerParser::LabelContext* ctx) override;
    void enterV_mov(VectorAssemblerParser::V_movContext* ctx) override;
    void enterV_add_f32(VectorAssemblerParser::V_add_f32Context* ctx) override;
    void enterV_sub_f32(VectorAssemblerParser::V_sub_f32Context* ctx) override;
    void enterV_mul_f32(VectorAssemblerParser::V_mul_f32Context* ctx) override;
    void enterV_div_f32(VectorAssemblerParser::V_div_f32Context* ctx) override;
    void enterV_cmp_eq_f32(VectorAssemblerParser::V_cmp_eq_f32Context* ctx) override;
    void enterS_mov(VectorAssemblerParser::S_movContext* ctx) override;
    void enterS_and(VectorAssemblerParser::S_andContext *ctx) override;
    void enterS_andn2(VectorAssemblerParser::S_andn2Context* ctx) override;
    void enterS_branch_vccz(VectorAssemblerParser::S_branch_vcczContext* ctx) override;
    void enterS_branch_vccnz(VectorAssemblerParser::S_branch_vccnzContext* ctx) override;
    void enterS_branch_execz(VectorAssemblerParser::S_branch_execzContext* ctx) override;
    void enterS_branch_execnz(VectorAssemblerParser::S_branch_execnzContext* ctx) override;

    const ParsedCode& GetParsedCode() const
    {
        return m_parsedCode;
    }

    bool HasError() const
    {
        return m_error;
    }

private:

    Instruction::Operand ParseOperand(antlr4::Token* token) const;
    Instruction::Operand ParseBranch(antlr4::Token* token) const;

    ParsedCode m_parsedCode;
    bool m_error = false;
};


class FrontEndErrorListener: public antlr4::BaseErrorListener 
{
public:
    void syntaxError(antlr4::Recognizer *recognizer, antlr4::Token *offendingSymbol,
                    size_t line, size_t charPositionInLine, const std::string &msg,
                    std::exception_ptr e) override;

    bool HasError() const
    {
        return m_error;
    }

private:
    bool m_error = false;
};


bool Parse(const char* filename, ParsedCode& parsedCode);