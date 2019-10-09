#pragma once
#include "antlr4-runtime/include/antlr4-runtime.h"
#include "VectorAssemblerLexer.h"
#include "VectorAssemblerParser.h"
#include "VectorAssemblerBaseListener.h"


static const uint32_t kMaxVectorRegisterIdx = 14;
static const uint32_t kMaxScalarRegisterIdx = 7;


enum InstructionType
{
    kInstructionTypeVector = 0,
    kInstructionTypeScalar = 1
};


#define INSTRUCTIONS(USER_DEFINE) \
    USER_DEFINE(kVectorMov,             "v_mov",            2, kInstructionTypeVector)\
    USER_DEFINE(kVectorAdd_f32,         "v_add_f32",        3, kInstructionTypeVector)\
    USER_DEFINE(kVectorSub_f32,         "v_sub_f32",        3, kInstructionTypeVector)\
    USER_DEFINE(kVectorMul_f32,         "v_mul_f32",        3, kInstructionTypeVector)\
    USER_DEFINE(kVectorDiv_f32,         "v_div_f32",        3, kInstructionTypeVector)\
    USER_DEFINE(kVectorCmpLt_f32,       "v_cmp_lt_f32",     2, kInstructionTypeVector)\
    USER_DEFINE(kVectorCmpLe_f32,       "v_cmp_le_f32",     2, kInstructionTypeVector)\
    USER_DEFINE(kVectorCmpEq_f32,       "v_cmp_eq_f32",     2, kInstructionTypeVector)\
    USER_DEFINE(kVectorCmpGt_f32,       "v_cmp_gt_f32",     2, kInstructionTypeVector)\
    USER_DEFINE(kVectorCmpGe_f32,       "v_cmp_ge_f32",     2, kInstructionTypeVector)\
    USER_DEFINE(kVectorCmpNe_f32,       "v_cmp_ne_f32",     2, kInstructionTypeVector)\
    USER_DEFINE(kVectorAdd_u32,         "v_add_u32",        3, kInstructionTypeVector)\
    USER_DEFINE(kVectorSub_u32,         "v_sub_u32",        3, kInstructionTypeVector)\
    USER_DEFINE(kVectorMul_u32,         "v_mul_u32",        3, kInstructionTypeVector)\
    USER_DEFINE(kVectorSll_u32,         "v_sll_u32",        3, kInstructionTypeVector)\
    USER_DEFINE(kVectorSrl_u32,         "v_srl_u32",        3, kInstructionTypeVector)\
    USER_DEFINE(kVectorAnd_u32,         "v_and_u32",        3, kInstructionTypeVector)\
    USER_DEFINE(kVectorAndNot_u32,      "v_andnot_u32",     3, kInstructionTypeVector)\
    USER_DEFINE(kVectorOr_u32,          "v_or_u32",         3, kInstructionTypeVector)\
    USER_DEFINE(kVectorXor_u32,         "v_xor_u32",        3, kInstructionTypeVector)\
    USER_DEFINE(kVectorCmpEq_u32,       "v_cmp_eq_u32",     2, kInstructionTypeVector)\
    USER_DEFINE(kVectorCmpGt_u32,       "v_cmp_gt_u32",     2, kInstructionTypeVector)\
    USER_DEFINE(kVectorCvtU32_F32,      "v_cvt_u32_f32",    2, kInstructionTypeVector)\
    USER_DEFINE(kVectorCvtF32_U32,      "v_cvt_f32_u32",    2, kInstructionTypeVector)\
    USER_DEFINE(kVectorLoad,            "v_load",           3, kInstructionTypeVector)\
    USER_DEFINE(kVectorStore,           "v_store",          3, kInstructionTypeVector)\
    USER_DEFINE(kScalarMov,             "s_mov",            2, kInstructionTypeScalar)\
    USER_DEFINE(kScalarAdd,             "s_add",            3, kInstructionTypeScalar)\
    USER_DEFINE(kScalarSub,             "s_sub",            3, kInstructionTypeScalar)\
    USER_DEFINE(kScalarAnd,             "s_and",            3, kInstructionTypeScalar)\
    USER_DEFINE(kScalarAndN2,           "s_andn2",          3, kInstructionTypeScalar)\
    USER_DEFINE(kScalarOr,              "s_or",             3, kInstructionTypeScalar)\
    USER_DEFINE(kScalarXor,             "s_xor",            3, kInstructionTypeScalar)\
    USER_DEFINE(kScalarShl,             "s_shl",            3, kInstructionTypeScalar)\
    USER_DEFINE(kScalarShr,             "s_shr",            3, kInstructionTypeScalar)\
    USER_DEFINE(kScalarLoad,            "s_load",           3, kInstructionTypeScalar)\
    USER_DEFINE(kScalarStore,           "s_store",          3, kInstructionTypeScalar)


enum EOpCode
{
    kInvalid = 0,

#define DEFINE_OP(ENUM_MEMBER, MNEMONIC, OPERANDS_NUM, TYPE) ENUM_MEMBER,
	INSTRUCTIONS(DEFINE_OP)
#undef DEFINE_OP

    kOpCodesNum
};


static const char* kOpToMnemonic[] = {
	"inv",
#define DEFINE_OPTOSTR(ENUM_MEMBER, MNEMONIC, OPERANDS_NUM, TYPE) MNEMONIC,
	INSTRUCTIONS(DEFINE_OPTOSTR)
#undef DEFINE_OPTOSTR
};


static uint32_t kOpOperandsNum[] = {
    0,
#define DEFINE_OPERANDS_NUM( ENUM_MEMBER, MNEMONIC, OPERANDS_NUM, TYPE) OPERANDS_NUM,
    INSTRUCTIONS(DEFINE_OPERANDS_NUM)
#undef DEFINE_OPERANDS_NUM
};


static InstructionType kOpToType[] = {
    kInstructionTypeVector,
#define DEFINE_TYPE( ENUM_MEMBER, MNEMONIC, OPERANDS_NUM, TYPE) TYPE,
    INSTRUCTIONS(DEFINE_TYPE)
#undef DEFINE_TYPE
};


struct Register
{
    enum Type : uint16_t
    {
        kVector = 0,
        kScalar,
        kScalar64,
        kVCC,
        kSCC,
        kEXEC
    };
    Type type;
    uint8_t idx;
    uint8_t rangeLen;

    bool operator==(const Register& r) const
    {
        return type == r.type && idx == r.idx;
    }

    bool operator!=(const Register& r) const
    {
        return type != r.type || idx != r.idx;
    }

    Register()
        : type(kVector), idx(0), rangeLen(0)
    {}

    Register(Type type, uint8_t idx)
        : type(type), idx(idx), rangeLen(0)
    {}
};
static_assert(sizeof(Register) == 4, "");


enum OperandType
{
    kOperandImmediate = 0,
    kOperandRegister = 1,
    kOperandLabelId = 2
};


struct Instruction
{
    EOpCode opCode;

    struct Operand
    {
        OperandType type;
        union
        {
            uint64_t imm;
            Register reg;
            const char* labelId;
        };

        Operand()
        {}
    };
    Operand operands[3];

    std::string label;
};


struct ParsedCode
{
    std::vector<Instruction> instructions;
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
    void enterV_cmp_lt_f32(VectorAssemblerParser::V_cmp_lt_f32Context* ctx) override;
    void enterV_cmp_le_f32(VectorAssemblerParser::V_cmp_le_f32Context* ctx) override;
    void enterV_cmp_eq_f32(VectorAssemblerParser::V_cmp_eq_f32Context* ctx) override;
    void enterV_cmp_gt_f32(VectorAssemblerParser::V_cmp_gt_f32Context* ctx) override;
    void enterV_cmp_ge_f32(VectorAssemblerParser::V_cmp_ge_f32Context* ctx) override;
    void enterV_cmp_ne_f32(VectorAssemblerParser::V_cmp_ne_f32Context* ctx) override;
    void enterV_add_u32(VectorAssemblerParser::V_add_u32Context* ctx) override;
    void enterV_sub_u32(VectorAssemblerParser::V_sub_u32Context* ctx) override;
    void enterV_mul_u32(VectorAssemblerParser::V_mul_u32Context* ctx) override;
    void enterV_sll_u32(VectorAssemblerParser::V_sll_u32Context* ctx) override;
    void enterV_srl_u32(VectorAssemblerParser::V_srl_u32Context* ctx) override;
    void enterV_and_u32(VectorAssemblerParser::V_and_u32Context* ctx) override;
    void enterV_andnot_u32(VectorAssemblerParser::V_andnot_u32Context* ctx) override;
    void enterV_or_u32(VectorAssemblerParser::V_or_u32Context* ctx) override;
    void enterV_xor_u32(VectorAssemblerParser::V_xor_u32Context* ctx) override;
    void enterV_cmp_eq_u32(VectorAssemblerParser::V_cmp_eq_u32Context* ctx) override;
    void enterV_cmp_gt_u32(VectorAssemblerParser::V_cmp_gt_u32Context* ctx) override;
    void enterV_cvt_u32_f32(VectorAssemblerParser::V_cvt_u32_f32Context* ctx) override;
    void enterV_cvt_f32_u32(VectorAssemblerParser::V_cvt_f32_u32Context* ctx) override;
    void enterV_load(VectorAssemblerParser::V_loadContext* ctx) override;
    void enterV_store(VectorAssemblerParser::V_storeContext* ctx) override;
    void enterS_mov(VectorAssemblerParser::S_movContext* ctx) override;
    void enterS_add(VectorAssemblerParser::S_addContext* ctx) override;
    void enterS_sub(VectorAssemblerParser::S_subContext* ctx) override;
    void enterS_and(VectorAssemblerParser::S_andContext *ctx) override;
    void enterS_andn2(VectorAssemblerParser::S_andn2Context* ctx) override;
    void enterS_or(VectorAssemblerParser::S_orContext* ctx) override;
    void enterS_xor(VectorAssemblerParser::S_xorContext* ctx) override;
    void enterS_shl(VectorAssemblerParser::S_shlContext* ctx) override;
    void enterS_shr(VectorAssemblerParser::S_shrContext* ctx) override;
    void enterS_load(VectorAssemblerParser::S_loadContext* ctx) override;
    void enterS_store(VectorAssemblerParser::S_storeContext* ctx) override;

    const ParsedCode& GetParsedCode() const
    {
        return m_parsedCode;
    }

    bool HasError() const
    {
        return m_error;
    }

private:

    Register ParseRegister(const std::string& str, size_t type);
    Instruction::Operand ParseOperand(antlr4::Token* token);
    Instruction::Operand ParseBranch(antlr4::Token* token);
    template<class CtxType>
    void Add2OpInstruction(EOpCode opCode, CtxType* ctx);
    template<class CtxType>
    void Add3OpInstruction(EOpCode opCode, CtxType* ctx);

    ParsedCode m_parsedCode;
    std::set<std::string> m_labelsVisit;
    std::string m_curLabel;
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