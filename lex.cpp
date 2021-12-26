#include "common.h"

#include "lex.h"

#define MaxNameLength 0x7f

inline bool IsValidNameFirstChar(uint c)
{
    return c == '_' || (c | 32u) - 'a' < 26u;
}

inline bool IsValidNameTrailingChar(uint c)
{
    return c == '_' || (c | 32u) - 'a' < 26u || c - '0' < 10u;
}


static void SetLexError(Token *token, LexErrorKind lexError)
{
    token->kind = Token_LexError; // both these stores could be done as one uint16 store.
    token->data.error.lexError = lexError;
}

static const ubyte *
FinishIntegerLiteral(const ubyte *p, Token *token, uint64_t raw)
{
    token->kind = Token_NumberLiteral;
    token->data.numberRawU64 = raw;

    token->numberLiteralBuiltinType = (raw >> 32) ? BuiltinType_g64 : BuiltinType_g32;

    if ((*p | 32) == 'u') {
        ++p;
        token->bNumberLiteralUnsigned = true;
    }

    if ((*p | 32) == 'l') {
        NotImplemented("long suffix");
    }

    return p;
}


/*
    Scan a raw token.

    Caller must ensure pSrcSentinel points to a '\0' ('\n' could be made to work too).
    
    The token is "raw", in that:
        - The final type of a numeric literal is not known.
        - A unary negate ('-') infront of a numeric literal may be a seperate token.
**/
TokenKind
Scanner_NextTokenRaw(Scanner *scanner, Token *token)
{
    ASSERT(*scanner->pSrcSentinel == '\0');

    const ubyte *const pSentinel = scanner->pSrcSentinel;
    const ubyte *p = scanner->pSrcCurr; // local, must assign at the end of all paths.
    uint c;

    ASSERT(p <= pSentinel);

    *token = { }; // zero
    token->lineno = scanner->lineno;

#if 0
    uint64_t mantissa;
    int exponent10;
#endif
    // Skip whitespace and comments:
    for (;;) {
        if (p >= pSentinel) {
            token->kind = Token_EOI;
            scanner->pSrcCurr = pSentinel;
            return Token_EOI;
        }
        c = *p++; // consume
        switch (c) {
            case '\n':
                scanner->lineno += 1;
                token->lineno = scanner->lineno;
                continue;
            case ' ': case '\r':
                continue;
            case '\t':
                continue; // something about tab alignment
            case '/': {
                uint const c1 = *p; // peek
                if (c1 == '/') {
                    do c = *++p; while (c != 0 && c != '\n');

                    continue;
                }
                else if (c1 == '*') {
                    ++p;
                    if (*p == '/') {
                        ++p; // A "/*/" should continue.
                    }
                    // p now points to fist thing that can be a '/' ender.
                    for (;;) {
                        if (p == pSentinel) {
                            SetLexError(token, LexError_BlockCommentNoEnd);
                            scanner->pSrcCurr = pSentinel;
                            return Token_LexError;
                        }
                        c = *p++; // consume
                        if (c == '\n') {
                            scanner->lineno += 1;
                            token->lineno = scanner->lineno;
                            continue;
                        }
                        if (c == '/' && p[-2] == '*') {
                            break;
                        }
                    }
                    continue; // keep looking for trailing whitespace or new comments.
                }
            } break; // case '/'
        } // switch
        break;
    } // loop
    ASSERT(p[-1] == c);

    switch (c) {
    case '*': {
        if (*p == '/') {
            SetLexError(token, LexError_BlockCommentNoBegin);
            break;
        }
        token->kind = Token_Mul;
    } break;
    case '!': if (*p == '=') { ++p; token->kind = Token_CmpNotEq;         } else { token->kind = Token_UnaryLogicalNot; }   break;
    case '=': if (*p == '=') { ++p; token->kind = Token_CmpEqual;         } else { token->kind = Token_Assign; }            break;
    case '+': if (*p == '+') { ++p; token->kind = Token_Inc;              } else { token->kind = Token_Plus; }              break;
    case '-': if (*p == '-') { ++p; token->kind = Token_Dec;              } else { token->kind = Token_Minus; }             break;
    case '&': if (*p == '&') { ++p; token->kind = Token_LogicAnd;         } else { token->kind = Token_Amp; }               break;
    case '|': if (*p == '|') { ++p; token->kind = Token_LogicOr;          } else { token->kind = Token_VBar; }              break;
    case '<': if (*p == '<') { ++p; token->kind = Token_LeftShift;        } else { token->kind = Token_LessThan; }          break; // TODO: <=
    case '>': if (*p == '>') { ++p; token->kind = Token_RightShift;       } else { token->kind = Token_GreaterThan; }       break; // TODO: >=
    case '^': token->kind = Token_Caret;            break;
    case '~': token->kind = Token_UnaryBitwiseNot;  break;
    case ',': token->kind = Token_Comma;            break;
    case ';': token->kind = Token_SemiColon;        break;
    case '{': token->kind = Token_OpenCurly;        break;
    case '}': token->kind = Token_CloseCurly;       break;
    case '(': token->kind = Token_OpenParen;        break;
    case ')': token->kind = Token_CloseParen;       break;
    case '0': {
        uint const c1 = *p;
        uint const lower = c1 | 32u;
        if (c1 - '0' < 10u) {
            NotImplemented("octal literals not supported"); 
        }
        else if (lower == 'x') {
            NotImplemented("hex literals"); 
        }
        else if (lower == 'b') {
            NotImplemented("binary literals");
        }
        else if (c1 == '.') {
            NotImplemented("floating point not supported"); 
        }
        else {
            p = FinishIntegerLiteral(p, token, 0);
        }
    } break;
    case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': {
        // Scan nonzero base 10 number.
        const ubyte *pMostSigDigit = p - 1; // when handle inner single quotes, need to have seperate counter
        uint64_t accum = c - '0';
        for (uint d; (d = *p - '0') < 10u; ++p) {
            accum = accum * 10u + d;
        }

        if (*p == '\'') {
            NotImplemented("number literal sep (1'000)");
        }

        if (*p == '.') {
            NotImplemented("FP literals");
        }
        else {
            p = FinishIntegerLiteral(p, token, accum);
        }

        // Check for uint64_t overflow. This is a a way I came up with that usually (for integers with < 20 digist) only does
        // a single comparison out of a loop, instead of something in a loop. msvc's std::from_chars<uint64_t> handles different bases nicely though, as does ion.
        uint const count = uint(p - pMostSigDigit); // p is after least sig digit
        if (count >= 20) { // unlikely
            // UINT64_MAX has 20 digits with a most sig digit of 1 (at exponent10==19).
            // For k in [1e19, 2e19 - 1], the max value of k only wraps once (quotient==1) mod 2^64,
            // and this warpped value is still lower than the min 20-digit number:
            if (count != 20 || *pMostSigDigit >= '2' || accum < 10'000'000'000'000'000'000u) {
                SetLexError(token, LexError_IntLiteralOver64Bits); // This is at the end as integer suffix still has to be consumed.
            }
        }
    } break;
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't':
    case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
    case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '_': {
        const ubyte *first = p - 1;
        while (IsValidNameTrailingChar(*p)) {
            p++;
        }
        uint32_t const n = uint32_t(p - first);
        if (n <= MaxNameLength) {
            token->kind = Token_Name;
            token->nameLength = n;
            token->data.nameBegin = first;
            // this sucks but lets get something going
            static struct {
                const char *str; TokenKind eToken;
            } KeyWords[] = {
            #define F(name) { #name, Token_Kw_##name }
                F(static_assert),
                F(void),
                F(char),
                F(bool),
                F(short),
                F(int),
                F(long),
                F(half),
                F(float),
                F(double),
            #undef F
            };
            for (const auto &entry : KeyWords) {
                uint i = 0;
                for (; i < n; ++i) {
                    if (first[i] != entry.str[i]) {
                        break;
                    }
                }
                if (entry.str[i] == 0) {
                    token->kind = entry.eToken;
                    break;
                }
            }
        }
        else {
            SetLexError(token, LexError_NameTooLong);
        }
    } break;
    default: {
        SetLexError(token, LexError_InvalidByte);
        token->data.error.invalidByte = c;
    } break;
    } // end switch

    scanner->pSrcCurr = p;
    return token->kind;
}
