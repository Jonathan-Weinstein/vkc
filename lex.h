#pragma once

#include <stdint.h>

enum TokenKind : uint8_t {
	Token_EOI,              // end of input
	Token_LexError,
	Token_Name,	            
	Token_DotName,	        
	Token_NumberLiteral,	// 0 0xf -5 1.5f
	Token_SemiColon,		// ;
	Token_Comma,		    // ,
	Token_OpenParen,		// (
	Token_CloseParen,		// )
	Token_OpenCurly,		// {
	Token_CloseCurly,		// }
	Token_Mul,		        // *
	Token_Plus,             // +
	Token_Minus,            // -
	Token_Inc,              // ++
	Token_Dec,              // --
    Token_Amp,              // &
    Token_Caret,            // ^
	Token_VBar,             // |
	Token_LogicAnd,         // &&
	Token_LogicOr,          // ||
	Token_LessThan,         // <
	Token_GreaterThan,      // >
	Token_LeftShift,        // <<
	Token_RightShift,       // >>
	Token_Assign,           // =
	Token_CmpEqual,         // ==
	Token_CmpNotEq,         // !=
	Token_UnaryLogicalNot,  // !
	Token_UnaryBitwiseNot,  // ~
	Token_Kw_static_assert, // static_assert
	Token_Kw_void,
	Token_Kw_char,
	Token_Kw_bool,
	Token_Kw_short,
	Token_Kw_int,
	Token_Kw_long,
	Token_Kw_half,
	Token_Kw_float,
	Token_Kw_double,
};

struct Token {
	TokenKind kind;
    BuiltinTypeKind numberLiteralBuiltinType; // only valid if Token_NumberLiteral
    bool bNumberLiteralUnsigned; // only valid if Token_NumberLiteral
    uint8_t nameLength; // Token_Name

    int32_t lineno;

    // Valid field is determined by this->type.
	union {
        uint64_t numberRawU64; // Token_NumberLiteral
        double numberDouble; // Token_NumberLiteral
        float numberFloat; // Token_NumberLiteral, maybe keep as double?

        const ubyte *nameBegin; // Token_Name, NOTE: points into the source text!

        struct {
            uint8_t invalidByte; // LexError_InvalidByte
            LexErrorKind lexError; // Only valid when (this->type==Token_LexError).
        } error; // Token_LexError
    } data;
};

struct Scanner
{
    const ubyte *pSrcCurr;
    const ubyte *pSrcSentinel;

    uint32_t lineno;
};

TokenKind
Scanner_NextTokenRaw(Scanner *scanner, Token *token);

inline void
Scanner_Init(Scanner *scanner, view<const char> input)
{
    scanner->pSrcCurr     = reinterpret_cast<const ubyte *>(input.ptr);
    scanner->pSrcSentinel = reinterpret_cast<const ubyte *>(input.end());
    scanner->lineno = 1;

    ASSERT(*scanner->pSrcSentinel == 0);
}
