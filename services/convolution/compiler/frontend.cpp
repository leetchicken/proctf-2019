#include "frontend.h"
#include <iostream>
#include <string.h>


void FrontEnd::enterLabel(VectorAssemblerParser::LabelContext* ctx) 
{
    m_curLabel = ctx->id->getText();
    if(m_labelsVisit.find(m_curLabel) != m_labelsVisit.end())
    {
        auto token = ctx->getStart();
        printf("error %u:%u: label '%s' is already defined\n", (uint32_t)token->getLine(), (uint32_t)token->getCharPositionInLine(), m_curLabel.c_str());
        m_curLabel.clear();
        m_error = true;
    }
    else
        m_labelsVisit.insert(m_curLabel);
}


void FrontEnd::enterV_mov(VectorAssemblerParser::V_movContext* ctx)
{
    Add2OpInstruction(kVectorMov, ctx);
}


void FrontEnd::enterV_add_f32(VectorAssemblerParser::V_add_f32Context* ctx)
{
    Add3OpInstruction(kVectorAdd_f32, ctx);
}


void FrontEnd::enterV_sub_f32(VectorAssemblerParser::V_sub_f32Context* ctx)
{
    Add3OpInstruction(kVectorSub_f32, ctx);
}


void FrontEnd::enterV_mul_f32(VectorAssemblerParser::V_mul_f32Context* ctx)
{
    Add3OpInstruction(kVectorMul_f32, ctx);
}


void FrontEnd::enterV_div_f32(VectorAssemblerParser::V_div_f32Context* ctx)
{
    Add3OpInstruction(kVectorDiv_f32, ctx);
}


void FrontEnd::enterV_cmp_lt_f32(VectorAssemblerParser::V_cmp_lt_f32Context* ctx)
{
    Add2OpInstruction(kVectorCmpLt_f32, ctx);
}


void FrontEnd::enterV_cmp_le_f32(VectorAssemblerParser::V_cmp_le_f32Context* ctx)
{
    Add2OpInstruction(kVectorCmpLe_f32, ctx);
}


void FrontEnd::enterV_cmp_eq_f32(VectorAssemblerParser::V_cmp_eq_f32Context* ctx) 
{
    Add2OpInstruction(kVectorCmpEq_f32, ctx);
}


void FrontEnd::enterV_cmp_gt_f32(VectorAssemblerParser::V_cmp_gt_f32Context* ctx) 
{
    Add2OpInstruction(kVectorCmpGt_f32, ctx);
}


void FrontEnd::enterV_cmp_ge_f32(VectorAssemblerParser::V_cmp_ge_f32Context* ctx)
{
    Add2OpInstruction(kVectorCmpGe_f32, ctx);
}


void FrontEnd::enterV_cmp_ne_f32(VectorAssemblerParser::V_cmp_ne_f32Context* ctx)
{
    Add2OpInstruction(kVectorCmpNe_f32, ctx);
}


void FrontEnd::enterV_add_u32(VectorAssemblerParser::V_add_u32Context* ctx)
{
    Add3OpInstruction(kVectorAdd_u32, ctx);
}


void FrontEnd::enterV_sub_u32(VectorAssemblerParser::V_sub_u32Context* ctx)
{
    Add3OpInstruction(kVectorSub_u32, ctx);
}


void FrontEnd::enterV_mul_u32(VectorAssemblerParser::V_mul_u32Context* ctx)
{
    Add3OpInstruction(kVectorMul_u32, ctx);
}


void FrontEnd::enterV_cmp_eq_u32(VectorAssemblerParser::V_cmp_eq_u32Context* ctx) 
{
    Add2OpInstruction(kVectorCmpEq_u32, ctx);
}


void FrontEnd::enterV_cmp_gt_u32(VectorAssemblerParser::V_cmp_gt_u32Context* ctx)
{
    Add2OpInstruction(kVectorCmpGt_u32, ctx);
}


void FrontEnd::enterV_cvt_u32_f32(VectorAssemblerParser::V_cvt_u32_f32Context* ctx)
{
    Add2OpInstruction(kVectorCvtU32_F32, ctx);
}


void FrontEnd::enterV_cvt_f32_u32(VectorAssemblerParser::V_cvt_f32_u32Context* ctx)
{
    Add2OpInstruction(kVectorCvtF32_U32, ctx);
}


void FrontEnd::enterV_load(VectorAssemblerParser::V_loadContext* ctx)
{
    Add3OpInstruction(kVectorLoad, ctx);
}


void FrontEnd::enterV_store(VectorAssemblerParser::V_storeContext* ctx)
{
    Add3OpInstruction(kVectorStore, ctx);
}


void FrontEnd::enterS_mov(VectorAssemblerParser::S_movContext* ctx) 
{
    Add2OpInstruction(kScalarMov, ctx);
}


void FrontEnd::enterS_addu(VectorAssemblerParser::S_adduContext* ctx)
{
    Add3OpInstruction(kScalarAddu, ctx);
}


void FrontEnd::enterS_and(VectorAssemblerParser::S_andContext *ctx)
{
    Add3OpInstruction(kScalarAnd, ctx);
}


void FrontEnd::enterS_andn2(VectorAssemblerParser::S_andn2Context* ctx)
{
    Add3OpInstruction(kScalarAndN2, ctx);
}


void FrontEnd::enterS_or(VectorAssemblerParser::S_orContext* ctx)
{
    Add3OpInstruction(kScalarOr, ctx);
}


void FrontEnd::enterS_shl(VectorAssemblerParser::S_shlContext* ctx)
{
    Add3OpInstruction(kScalarShl, ctx);
}


void FrontEnd::enterS_shr(VectorAssemblerParser::S_shrContext* ctx)
{
    Add3OpInstruction(kScalarShr, ctx);
}


void FrontEnd::enterS_load(VectorAssemblerParser::S_loadContext* ctx)
{
    Add3OpInstruction(kScalarLoad, ctx);
}


void FrontEnd::enterS_store(VectorAssemblerParser::S_storeContext* ctx)
{
    Add3OpInstruction(kScalarStore, ctx);
}


void FrontEnd::enterS_branch_vccz(VectorAssemblerParser::S_branch_vcczContext* ctx) 
{
    Instruction instr;
    instr.opCode = kScalarBranchVCCZ;
    instr.operands[0] = ParseBranch(ctx->label_id);
    if(!m_curLabel.empty())
    {
        instr.label = m_curLabel;
        m_curLabel.clear();
    }
    m_parsedCode.instructions.push_back(instr);
}


void FrontEnd::enterS_branch_vccnz(VectorAssemblerParser::S_branch_vccnzContext* ctx) 
{
    Instruction instr;
    instr.opCode = kScalarBranchVCCNZ;
    instr.operands[0] = ParseBranch(ctx->label_id);
    if(!m_curLabel.empty())
    {
        instr.label = m_curLabel;
        m_curLabel.clear();
    }
    m_parsedCode.instructions.push_back(instr);
}


void FrontEnd::enterS_branch_execz(VectorAssemblerParser::S_branch_execzContext* ctx) 
{
    Instruction instr;
    instr.opCode = kScalarBranchEXECZ;
    instr.operands[0] = ParseBranch(ctx->label_id);
    if(!m_curLabel.empty())
    {
        instr.label = m_curLabel;
        m_curLabel.clear();
    }
    m_parsedCode.instructions.push_back(instr);
}


void FrontEnd::enterS_branch_execnz(VectorAssemblerParser::S_branch_execnzContext* ctx) 
{
    Instruction instr;
    instr.opCode = kScalarBranchEXECNZ;
    instr.operands[0] = ParseBranch(ctx->label_id);
    if(!m_curLabel.empty())
    {
        instr.label = m_curLabel;
        m_curLabel.clear();
    }
    m_parsedCode.instructions.push_back(instr);
}


Register FrontEnd::ParseRegister(const std::string& str, size_t type)
{
    Register reg;

    if(type == VectorAssemblerParser::VREGISTER_RANGE)
    {
        reg.type = Register::kVector;
        char buf[32];
        strcpy(buf, str.c_str());
        char* delim = strchr(buf, ':');
        *delim = '\0';
		reg.idx = strtoul(&buf[2], nullptr, 10);
        uint32_t reg2 = strtoul(&delim[1], nullptr, 10);
        reg.rangeLen = reg2 - reg.idx;
    }
    else
    {
        if(type == VectorAssemblerParser::VREGISTER)
            reg.type = Register::kVector;
        else if(type == VectorAssemblerParser::SREGISTER)
            reg.type = Register::kScalar;
        else if(type == VectorAssemblerParser::SREGISTER64)
            reg.type = Register::kScalar64;

        const char* idxStr = str.data() + 1;
	    reg.idx = strtoul(idxStr, nullptr, 10);
        reg.rangeLen = 0;
    }

    if(reg.type == Register::kVector)
    {
        if(reg.idx > kMaxVectorRegisterIdx)
        {
            printf("Invalid vector register: '%s'\n", str.c_str());
            m_error = true;
        }
        if(reg.idx + reg.rangeLen > kMaxVectorRegisterIdx)
        {
            printf("Invalid vector register range: '%s'\n", str.c_str());
            m_error = true;
        }
    }
    else if(reg.type == Register::kScalar || reg.type == Register::kScalar64)
    {
        if(reg.idx > kMaxScalarRegisterIdx)
        {
            printf("Invalid scalar register: '%s'\n", str.c_str());
            m_error = true;
        }
    }

    return reg;
}


Instruction::Operand FrontEnd::ParseOperand(antlr4::Token* token)
{
    Instruction::Operand op;
    if(!token)
    {
        m_error = true;
        return op;
    }

    auto type = token->getType();
    if(type == VectorAssemblerParser::VREGISTER || type == VectorAssemblerParser::SREGISTER || 
       type == VectorAssemblerParser::SREGISTER64 || type == VectorAssemblerParser::VREGISTER_RANGE)
    {
        op.type = kOperandRegister;
        op.reg = ParseRegister(token->getText(), type);
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
    else if(type == VectorAssemblerParser::SCC)
    {
        op.type = kOperandRegister;
        op.reg.type = Register::kSCC;
    }
    else if(type == VectorAssemblerParser::DECIMAL)
    {
        op.type = kOperandImmediate;
        auto str = token->getText();
        op.imm = strtoull(str.c_str(), nullptr, 10);
    }
    else if(type == VectorAssemblerParser::HEXADECIMAL)
    {
        op.type = kOperandImmediate;
        auto str = token->getText();
        op.imm = strtoull(str.c_str(), nullptr, 16);
    }
    else if(type == VectorAssemblerParser::FLOAT)
    {
        op.type = kOperandImmediate;
        auto str = token->getText();
        float f = strtof(str.c_str(), nullptr);
        op.imm = 0;
        memcpy(&op.imm, &f, sizeof(float));
    }

    return op;
}


Instruction::Operand FrontEnd::ParseBranch(antlr4::Token* token)
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


template<class CtxType>
void FrontEnd::Add2OpInstruction(EOpCode opCode, CtxType* ctx)
{
    Instruction instr;
    instr.opCode = opCode;
    instr.operands[0] = ParseOperand(ctx->op0);
    instr.operands[1] = ParseOperand(ctx->op1);
    if(!m_curLabel.empty())
    {
        instr.label = m_curLabel;
        m_curLabel.clear();
    }
    m_parsedCode.instructions.push_back(instr);
}


template<class CtxType>
void FrontEnd::Add3OpInstruction(EOpCode opCode, CtxType* ctx)
{
    Instruction instr;
    instr.opCode = opCode;
    instr.operands[0] = ParseOperand(ctx->op0);
    instr.operands[1] = ParseOperand(ctx->op1);
    instr.operands[2] = ParseOperand(ctx->op2);
    if(!m_curLabel.empty())
    {
        instr.label = m_curLabel;
        m_curLabel.clear();
    }
    m_parsedCode.instructions.push_back(instr);
}


void FrontEndErrorListener::syntaxError(antlr4::Recognizer *recognizer, antlr4::Token *offendingSymbol,
                                        size_t line, size_t charPositionInLine, const std::string &msg,
                                        std::exception_ptr e) 
{
    printf("error %u:%u: %s\n", (uint32_t)line, (uint32_t)charPositionInLine, msg.c_str());
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
