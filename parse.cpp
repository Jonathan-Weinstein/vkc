#include "common.h"
#include "message.h"
#include "lex.h"
#include "type.h"

#include <string.h>
#include <stdio.h> // devel

enum TypelessOp : uint8_t {
    TypelessOp_UnaryPlus, // just typechecks is arithmetic and nulls out Variable ref (leaving Value)
    TypelessOp_UnaryNegate,
    TypelessOp_UnaryLogicalNot,
    TypelessOp_UnaryBitwiseNot,
    TypelessOp_BitwiseAnd,
    TypelessOp_BitwiseOr,
    TypelessOp_BitwiseXor,
    TypelessOp_Add,
    TypelessOp_Sub,
    TypelessOp_Mul,
    TypelessOp_CmpEqual,
    TypelessOp_CmpNotEq,
    TypelessOp_Assign,
    // TypelessOp_Index, // transformed 
    TypelessOp_CallOrFunctionalCast, // transformed 
#define TypelessOp_LowEnumEnd (TypelessOp_CallOrFunctionalCast + 1)
    TypelessOp_OpenParen,
    TypelessOp_CloseParen,
#define TypelessOp_ParserEnumEnd (TypelessOp_CloseParen + 1)
};
constexpr uint IsRightAssocFlag = 1u << 8, PrecShift = 9u;
// https://en.cppreference.com/w/cpp/language/operator_precedence
// GLSL is similar: https://www.khronos.org/registry/OpenGL/specs/gl/GLSLangSpec.4.60.html#operators
// NOTE: those docs have lower values for higher, but do DoStackedOp() wants the other way, hence the (31 - x).
enum OpInfo : uint16_t {
    // NOTE: there is no OpInfo for any of ")]}:", shuold be done immediately.

    // 1:                           = (31 - 1)

    // 2:
    OpInfo_CallOrFunctionCast   = (31 - 2) << PrecShift | TypelessOp_CallOrFunctionalCast,

    // 3, right assoc:
    OpInfo_UnaryPlus            = (31 - 3) << PrecShift | TypelessOp_UnaryPlus          | IsRightAssocFlag,
    OpInfo_UnaryNegate          = (31 - 3) << PrecShift | TypelessOp_UnaryNegate        | IsRightAssocFlag,
    OpInfo_UnaryLogicalNot      = (31 - 3) << PrecShift | TypelessOp_UnaryLogicalNot    | IsRightAssocFlag,
    OpInfo_UnaryBitwiseNot      = (31 - 3) << PrecShift | TypelessOp_UnaryBitwiseNot    | IsRightAssocFlag,

    // 4:                           = (31 - 4)

    // 5:
    OpInfo_Mul                  = (31 - 5) << PrecShift | TypelessOp_Mul,

    // 6:
    OpInfo_Add                  = (31 - 6) << PrecShift | TypelessOp_Add,
    OpInfo_Sub                  = (31 - 6) << PrecShift | TypelessOp_Sub,

    // 7:                           = (31 - 7)
    // 8:                           = (31 - 8)
    // 9:                           = (31 - 9)

    //10:
    OpInfo_CmpEqual             = (31 -10) << PrecShift | TypelessOp_CmpEqual,
    OpInfo_CmpNotEq             = (31 -10) << PrecShift | TypelessOp_CmpNotEq,

    //11:
    OpInfo_BitwiseAnd           = (31 -11) << PrecShift | TypelessOp_BitwiseAnd,

    //12:
    OpInfo_BitwiseXor           = (31 -12) << PrecShift | TypelessOp_BitwiseXor,

    //13:
    OpInfo_BitwiseOr            = (31 -13) << PrecShift | TypelessOp_BitwiseOr,

    //14:                           = (31 -14)
    //15:                           = (31 -15)

    //16, right assoc:
    OpInfo_Assign               = (31 -16) << PrecShift | TypelessOp_Assign     | IsRightAssocFlag,

    //17:                           = (31 -17)

    //18: group openings
    OpInfo_OpenParen            = (31 -18) << PrecShift | TypelessOp_OpenParen,

    //special:
    OpInfo_StackStartMinPrecSentinel = 0,
    OpInfo_FinalCollapse             = 1,
    OpInfo_Invalid                   = 0x7fff
};
static TypelessOp GetTypelessOp(OpInfo info)
{
    return TypelessOp(uint8_t(info));
}
#if 0 // dont have a sep OpKind for compund ops, handle += as + followed by a =
static bool IsCompoundAssign(OpInfo info)
{
    return false;
}
#endif
static bool DoStackedOp(OpInfo stacked, OpInfo incoming)
{
    static_assert(OpInfo_StackStartMinPrecSentinel < OpInfo_FinalCollapse, "");
    ASSERT((stacked >> PrecShift) != 0 || stacked == OpInfo_StackStartMinPrecSentinel);
    ASSERT((incoming >> PrecShift) != 0 || incoming == OpInfo_FinalCollapse);

    return (stacked & ~IsRightAssocFlag) >= incoming;
}

static bool IsUnary(TypelessOp kind)
{
    enum : uint32_t { YesBits = 
        1u << TypelessOp_CallOrFunctionalCast |
        1u << TypelessOp_UnaryPlus |
        1u << TypelessOp_UnaryNegate |
        1u << TypelessOp_UnaryLogicalNot |
        1u << TypelessOp_UnaryBitwiseNot
    };
    return YesBits >> kind & 1;
}


// Could also just store this in lexer token struct since it knows what TokenKind it fills in:
static OpInfo OpInfoFromToken(TokenKind t)
{
    switch (t) {
    default:
        ASSERT(0);
        unreachable;
    case Token_UnaryLogicalNot: return OpInfo_UnaryLogicalNot;
    case Token_UnaryBitwiseNot: return OpInfo_UnaryBitwiseNot;

    /* Only call with these if known to be non-unary: */
    case Token_Mul: return OpInfo_Mul;
    case Token_Plus: return OpInfo_Add; 
    case Token_Minus: return OpInfo_Sub;

    case Token_CmpEqual: return OpInfo_CmpEqual;
    case Token_CmpNotEq: return OpInfo_CmpNotEq;
    case Token_Amp: return OpInfo_BitwiseAnd;
    case Token_Caret: return OpInfo_BitwiseXor;
    case Token_VBar: return OpInfo_BitwiseOr;
    case Token_Assign: return OpInfo_Assign;
    case Token_OpenParen: return OpInfo_OpenParen;
    }
}

Message *MessageStream::PushRaw()
{
    uint n = this->nMessages;
    if (n >= lengthof(this->messages)) { NotImplemented("too many messages"); }
    return &this->messages[this->nMessages++];
}


enum { TokenBufModMask = 7u };

struct Context {

    Scanner scanner;
    uint8_t peekIndex;
    Token tokenbuf[TokenBufModMask + 1]; // LL(1), so in theory array size could be 2 [2]. NOTE: calling GetAndAdvance() invalidate's all pointers... may screw myself here.

    // Could "move" stuff in here to avoid indirections for things like oms, then move back at the end.
    MessageStream *oms = nullptr;

    Context() = default;
    Context(const Context&) = delete;
    void operator=(const Context&) = delete;
};

static const Token *
GetAndAdvance(Context *ctx)
{
    static_assert(((TokenBufModMask + 1) & TokenBufModMask) == 0 && TokenBufModMask, "");
    uint oldPeek = ctx->peekIndex, newPeek;
    ctx->peekIndex = newPeek = ((oldPeek + 1) & TokenBufModMask);
    Scanner_NextTokenRaw(&ctx->scanner, &ctx->tokenbuf[newPeek]);
    printf("GetAndAdvance: got %d\n", ctx->tokenbuf[oldPeek].kind);
    return &ctx->tokenbuf[oldPeek];
}

static const Token *
Peek(const Context *ctx)
{
    return &ctx->tokenbuf[ctx->peekIndex];
}

static void
Expect(Context *ctx, TokenKind eToken)
{
    if (GetAndAdvance(ctx)->kind != eToken) {
        NotImplemented("Handle Expect(token) mismatch");
    }
}


struct ImmediateData {
    union {
        //uint64_t v2u64[2];
        //uint32_t v4u32[4];
        //float v4f32[4];

        uint64_t u64; // used for bool ops too
        uint64_t s64; // used for bool ops too
        float f32;
    } small;
};

// ParseOpArg flags:
enum : uint8_t {
    ArgFlagImmediate                   = 1u << 0, // no SpvId(s) made for constants yet, stored inline
    // FlagScalarizedVector         = 1u << 1, // if set, means doing operations as N scalar ops instead of 1 vector op
    /* Only used during parsing: */
    ArgFlagResultOfComparsion          = 1u << 2, // used to warn against (a & b == c), likely intended ((a & b) == c). Cmp ops have higher prec than binop bitwise.
    ArgFlagAllowImplicitCvtToBool      = 1u << 3, // literals and vars/names of type scalar integer or pointer with no operators applied, except for ().
    ArgFlagResultOfAssignment          = 1u << 4,
    ArgFlagResultOfDiscardableFuncRet  = 1u << 5,
};

struct ParseOpArg {
    TypeDescriptor typedesc;
    uint8_t flags;
    ImmediateData imm;
};

enum ExprModeEnum : uint8_t {
    ExprMode_Regular,
    ExprMode_Immediate,
    ExprMode_TernarySelect,
    ExprMode_LogicalAnd,
    ExprMode_LogicalOr,
};


struct ParsedExprResult {
    ParseOpArg arg;
};


enum {
    ExprParseFlagMustBeConstexpr = 1u << 0,
    ExprParseFlagCommaContinues = 1u << 1
};

static void
ParseExpr(Context *ctx, ParsedExprResult *result, uint exprParseFlags)
{
    enum { MaxArgs = 32, MaxOps = 32 };
    ParseOpArg args[MaxArgs];
    OpInfo ops[MaxOps]; // might want line info too.
    ParseOpArg *argsEnd = args;
    // NOTE:
    OpInfo *opsEnd = ops + 1;
    ops[0] = OpInfo_StackStartMinPrecSentinel;

    int groupOpeningsEnd = 0;

    auto const CollapseSubexpr = [&](OpInfo incomingInfo) {
        ASSERT(incomingInfo != OpInfo_Invalid);
        for (OpInfo stackedInfo; DoStackedOp((stackedInfo = opsEnd[-1]), incomingInfo); --opsEnd) {
            TypelessOp const op = GetTypelessOp(stackedInfo);
            if (IsUnary(op)) {
                int64_t x = argsEnd[-1].imm.small.s64;
                int64_t r;
                switch (op) {
                case TypelessOp_UnaryPlus:       r = +x; break;
                case TypelessOp_UnaryNegate:     r = -x; break;
                case TypelessOp_UnaryLogicalNot: r = !x; break;
                case TypelessOp_UnaryBitwiseNot: r = ~x; break;
                default: ASSERT(0);
                }
                argsEnd[-1].imm.small.s64 = r;
                ASSERT(argsEnd[-1].flags & ArgFlagImmediate);
            }
            else { // assume binary for now
                ASSERT(argsEnd - args >= 2);
                int64_t a = argsEnd[-2].imm.small.s64;
                int64_t b = argsEnd[-1].imm.small.s64;
                int64_t c;
                switch (op) {
                case TypelessOp_Add:        c = a + b; break;
                case TypelessOp_Sub:        c = a - b; break;
                case TypelessOp_Mul:        c = a * b; break;
                case TypelessOp_BitwiseAnd: c = a & b; break;
                case TypelessOp_BitwiseXor: c = a ^ b; break;
                case TypelessOp_BitwiseOr:  c = a | b; break;
                case TypelessOp_CmpEqual:   c = (a == b), printf("%lld == %lld\n", a, b); break;
                case TypelessOp_CmpNotEq:   c = (a != b), printf("%lld != %lld\n", a, b); break;
                default: ASSERT(0);
                }
                argsEnd -= 1;
                argsEnd[-1].imm.small.s64 = c;
                ASSERT(argsEnd[-1].flags & ArgFlagImmediate);
            }
        }
        ASSERT(opsEnd > ops); // above sentinel
        *opsEnd++ = incomingInfo;
    };

    bool bLastWasArgOrGroupClose = false; // maybe have on stack that is union of op and arg?
    for (;;) {
        ASSERT(argsEnd <= args + MaxArgs);
        ASSERT(opsEnd <= ops + MaxOps);
        if (argsEnd == args + MaxArgs) {
            NotImplemented("");
        }
        if (opsEnd == ops + MaxOps) {
            NotImplemented("");
        }

        OpInfo incomingInfo = OpInfo_Invalid;
        const Token *tok = Peek(ctx);
        switch (tok->kind) {
        case Token_Comma: {
            if (!(exprParseFlags & ExprParseFlagCommaContinues)) {
                goto endloop;
            }
            NotImplemented("");
        } break;
        case Token_NumberLiteral: {
            if (bLastWasArgOrGroupClose) {
                NotImplemented("syntax error");
            }
            bLastWasArgOrGroupClose = true;
            ParseOpArg *arg = argsEnd++;
            arg->typedesc = MakeLeafTypeDesc(tok->numberLiteralBuiltinType, tok->bNumberLiteralUnsigned ? TypeDescLeafFlag_Unsigned : 0);
            arg->flags = ArgFlagImmediate;
            arg->imm.small.s64 = tok->data.numberRawU64;
            GetAndAdvance(ctx);
            continue; // NOTE
        } break;
        case Token_Plus:
        case Token_Minus:
            if (!bLastWasArgOrGroupClose) { // is unary?
                // bLastWasArgOrGroupClose still false, desired
                incomingInfo = tok->kind == Token_Plus ? OpInfo_UnaryPlus : OpInfo_UnaryNegate;
                GetAndAdvance(ctx);
                break;
            } // else, fallthru:
        case Token_Mul:
        case Token_Caret:
        case Token_VBar:
        case Token_Amp:
        case Token_CmpEqual:
        case Token_CmpNotEq: {
            bLastWasArgOrGroupClose = false;
            incomingInfo = OpInfoFromToken(tok->kind);
            GetAndAdvance(ctx);
        } break;
        case Token_OpenParen: {
            NotImplemented("");
        } break;
        case Token_CloseParen: {
            // could also handle a parenthesized expr by first pushing a '(', or just checking for it
            if (groupOpeningsEnd <= 0) {
                goto endloop;
            }
            NotImplemented("");
        } break;
        default: {
            goto endloop;
        }
        } // end switch
        CollapseSubexpr(incomingInfo);
    }
endloop:
    CollapseSubexpr(OpInfo_FinalCollapse);
    ASSERT(argsEnd == args + 1);
    ASSERT(opsEnd == ops + 2 && ops[0] == OpInfo_StackStartMinPrecSentinel && ops[1] == OpInfo_FinalCollapse);
    result->arg = args[0];
    ASSERT(result->arg.flags & ArgFlagImmediate);
    printf("result.s64 = %lld\n", (long long)result->arg.imm.small.s64);
}
// __data_u64[]
// abs()
static void
ParseParenthesizedExpr(Context *ctx, ParsedExprResult *result, uint exprParseFlags)
{
    Expect(ctx, Token_OpenParen);
    ParseExpr(ctx, result, exprParseFlags);
    Expect(ctx, Token_CloseParen);
}


void Compile(view<const char> source, MessageStream *oms)
{
    ASSERT(source.length >= 20);
    constexpr view<const char> prefix = "void main(){"_view;
    static_assert(prefix.length == 12, "");
    ASSERT(memcmp(source.ptr, prefix.ptr, prefix.length) == 0);
    source.ptr += 12;
    source.length -= 12;

    Context ctx;
    ctx.oms = oms;

    Scanner_Init(&ctx.scanner, source);
    ctx.peekIndex = 0;
    Scanner_NextTokenRaw(&ctx.scanner, &ctx.tokenbuf[0]);

    for (;;) {
        const Token t = *GetAndAdvance(&ctx); // copy

        switch (t.kind) {
        case Token_EOI:
            return;
        case Token_CloseCurly:
            return;
        case Token_Kw_static_assert: {
            ParsedExprResult result;
            auto *pStart = ctx.scanner.pSrcCurr - 1;
            ParseParenthesizedExpr(&ctx, &result, ExprParseFlagMustBeConstexpr);
            auto *pEnd = ctx.scanner.pSrcCurr;
            Expect(&ctx, Token_SemiColon);

            if (!(result.arg.flags & ArgFlagImmediate)) {
                NotImplemented("not constexpr");
            }

            // TODO: check is bool or implcitly convertible to bool:
            
            if (!result.arg.imm.small.s64) {
                Message *m = oms->PushRaw();
                *m = { };
                m->line = t.lineno;
                m->type = Message_StaticAssertFailed;
                 // debug
                printf("static_assert failed on line %d: ", t.lineno);
                fwrite(pStart, 1, pEnd - pStart, stdout);
                puts("");
            }
        } break;

        default:
            NotImplemented("Unexpected token at start of function");
        }
    }
}
