#include "frontend.h"
#include <iostream>
#include <string.h>


static Register ParseRegister(const std::string& str)
{
    Register reg;
    char regType = str[0];

    reg.type = regType == 'v' ? Register::kVector : Register::kScalar;
    const char* idxStr = str.data() + 1;
	reg.idx = strtoul(idxStr, nullptr, 10);
    return reg;
}


void FrontEnd::enterLabel(VectorAssemblerParser::LabelContext* ctx) 
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


void FrontEnd::enterV_mov(VectorAssemblerParser::V_movContext* ctx)
{
    Instruction instr;
    instr.opCode = kVectorMov;
    instr.type = kInstructionTypeVector;
    instr.operands[0] = ParseOperand(ctx->op0);
    instr.operands[1] = ParseOperand(ctx->op1);
    m_parsedCode.instructions.push_back(instr);
}


void FrontEnd::enterV_add_f32(VectorAssemblerParser::V_add_f32Context* ctx)
{
    Instruction instr;
    instr.opCode = kVectorAdd_f32;
    instr.type = kInstructionTypeVector;
    instr.operands[0] = ParseOperand(ctx->op0);
    instr.operands[1] = ParseOperand(ctx->op1);
    instr.operands[2] = ParseOperand(ctx->op2);
    m_parsedCode.instructions.push_back(instr);
}


void FrontEnd::enterV_sub_f32(VectorAssemblerParser::V_sub_f32Context* ctx)
{
    Instruction instr;
    instr.opCode = kVectorSub_f32;
    instr.type = kInstructionTypeVector;
    instr.operands[0] = ParseOperand(ctx->op0);
    instr.operands[1] = ParseOperand(ctx->op1);
    instr.operands[2] = ParseOperand(ctx->op2);
    m_parsedCode.instructions.push_back(instr);
}


void FrontEnd::enterV_mul_f32(VectorAssemblerParser::V_mul_f32Context* ctx)
{
    Instruction instr;
    instr.opCode = kVectorMul_f32;
    instr.type = kInstructionTypeVector;
    instr.operands[0] = ParseOperand(ctx->op0);
    instr.operands[1] = ParseOperand(ctx->op1);
    instr.operands[2] = ParseOperand(ctx->op2);
    m_parsedCode.instructions.push_back(instr);
}


void FrontEnd::enterV_div_f32(VectorAssemblerParser::V_div_f32Context* ctx)
{
    Instruction instr;
    instr.opCode = kVectorDiv_f32;
    instr.type = kInstructionTypeVector;
    instr.operands[0] = ParseOperand(ctx->op0);
    instr.operands[1] = ParseOperand(ctx->op1);
    instr.operands[2] = ParseOperand(ctx->op2);
    m_parsedCode.instructions.push_back(instr);
}


void FrontEnd::enterV_cmp_eq_f32(VectorAssemblerParser::V_cmp_eq_f32Context* ctx) 
{
    Instruction instr;
    instr.opCode = kVectorCmpEq_f32;
    instr.type = kInstructionTypeVector;
    instr.operands[0] = ParseOperand(ctx->op0);
    instr.operands[1] = ParseOperand(ctx->op1);
    m_parsedCode.instructions.push_back(instr);
}


void FrontEnd::enterS_mov(VectorAssemblerParser::S_movContext* ctx) 
{
    Instruction instr;
    instr.opCode = kScalarMov;
    instr.type = kInstructionTypeScalar;
    instr.operands[0] = ParseOperand(ctx->op0);
    instr.operands[1] = ParseOperand(ctx->op1);
    m_parsedCode.instructions.push_back(instr);
}


void FrontEnd::enterS_and(VectorAssemblerParser::S_andContext *ctx)
{
    Instruction instr;
    instr.opCode = kScalarAnd;
    instr.type = kInstructionTypeScalar;
    instr.operands[0] = ParseOperand(ctx->op0);
    instr.operands[1] = ParseOperand(ctx->op1);
    instr.operands[2] = ParseOperand(ctx->op2);
    m_parsedCode.instructions.push_back(instr);
}


void FrontEnd::enterS_andn2(VectorAssemblerParser::S_andn2Context* ctx)
{
    Instruction instr;
    instr.opCode = kScalarAndN2;
    instr.type = kInstructionTypeScalar;
    instr.operands[0] = ParseOperand(ctx->op0);
    instr.operands[1] = ParseOperand(ctx->op1);
    instr.operands[2] = ParseOperand(ctx->op2);
    m_parsedCode.instructions.push_back(instr);
}


void FrontEnd::enterS_branch_vccz(VectorAssemblerParser::S_branch_vcczContext* ctx) 
{
    Instruction instr;
    instr.opCode = kScalarBranchVCCZ;
    instr.type = kInstructionTypeScalar;
    instr.operands[0] = ParseBranch(ctx->label_id);
    m_parsedCode.instructions.push_back(instr);
}


void FrontEnd::enterS_branch_vccnz(VectorAssemblerParser::S_branch_vccnzContext* ctx) 
{
    Instruction instr;
    instr.opCode = kScalarBranchVCCNZ;
    instr.type = kInstructionTypeScalar;
    instr.operands[0] = ParseBranch(ctx->label_id);
    m_parsedCode.instructions.push_back(instr);
}


void FrontEnd::enterS_branch_execz(VectorAssemblerParser::S_branch_execzContext* ctx) 
{
    Instruction instr;
    instr.opCode = kScalarBranchEXECZ;
    instr.type = kInstructionTypeScalar;
    instr.operands[0] = ParseBranch(ctx->label_id);
    m_parsedCode.instructions.push_back(instr);
}


void FrontEnd::enterS_branch_execnz(VectorAssemblerParser::S_branch_execnzContext* ctx) 
{
    Instruction instr;
    instr.opCode = kScalarBranchEXECNZ;
    instr.type = kInstructionTypeScalar;
    instr.operands[0] = ParseBranch(ctx->label_id);
    m_parsedCode.instructions.push_back(instr);
}


Instruction::Operand FrontEnd::ParseOperand(antlr4::Token* token) const
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

Instruction::Operand FrontEnd::ParseBranch(antlr4::Token* token) const
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


void FrontEndErrorListener::syntaxError(antlr4::Recognizer *recognizer, antlr4::Token *offendingSymbol,
                                        size_t line, size_t charPositionInLine, const std::string &msg,
                                        std::exception_ptr e) 
{
    printf("error %u:%u: %s\n", line, charPositionInLine, msg.c_str());
    m_error = true;
}


bool Parse(const char* filename, ParsedCode& parsedCode)
{
    std::ifstream stream;
    stream.open(filename);
    antlr4::ANTLRInputStream input(stream);
    VectorAssemblerLexer lexer(&input);
    antlr4::CommonTokenStream tokens(&lexer);
    FrontEndErrorListener errorListener;
    VectorAssemblerParser parser(&tokens);
    parser.removeErrorListeners();
    parser.addErrorListener(&errorListener);

    antlr4::tree::ParseTree *tree = parser.start();
    FrontEnd frontEnd;
    antlr4::tree::ParseTreeWalker::DEFAULT.walk(&frontEnd, tree);

    parsedCode = frontEnd.GetParsedCode();
    return !errorListener.HasError() && !frontEnd.HasError();
}