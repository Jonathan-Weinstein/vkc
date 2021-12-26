#include "common.h"

#include "message.h"
#include "lex.h"

#include <stdio.h>
#include <stdlib.h> // strtoull, skips ws and checks for unary + or -
#include <errno.h> // strtoull sets errno to ERANGE https://en.cppreference.com/w/cpp/string/byte/strtoul
//#include <initializer_list>

void Scanner_TestRaw()
{
    puts(__FUNCTION__);

    {
        constexpr view<const char> input = " 0 8 456 18446744073709551615 18446744073709551616 28446744073709551615  "_view;

        Scanner scanner;
        Scanner_Init(&scanner, input);
      
        const char *iter = input.ptr;
        Token tok;
        for (;;) {
            char *strend;
            uint64_t const crtResult = strtoull(iter, &strend, 0);
            // printf("'%s'\n", iter);
            Scanner_NextTokenRaw(&scanner, &tok);
            if (iter == strend) {
                break;
            }

            int const e = errno;

            if (tok.kind == Token_NumberLiteral) {
                ASSERT(e == 0);

                ASSERT(crtResult == tok.data.numberRawU64);
            }
            else {
                fputs("overflow: ", stdout);
                fwrite(iter, 1, strend - iter, stdout);
                putchar('\n');
                ASSERT(tok.kind == Token_LexError);
                ASSERT(tok.data.error.lexError == LexError_IntLiteralOver64Bits);
                ASSERT(e == ERANGE);
            }

            iter = strend;
        }

        // check line numvers later.

        ASSERT(tok.kind == Token_EOI);
    }

    // test comment skipping
    {
        Scanner sc;
        Scanner_Init(&sc, R"(
0
1 2 /// 999 //#%$*#$*
  // ???
3 /*//********** %%%     */      4
/* /* 
777
**/ 5  )"_view);

        uint i = 0;
        Token tok;
        while (Scanner_NextTokenRaw(&sc, &tok), tok.kind == Token_NumberLiteral) {
            ASSERT(i == tok.data.numberRawU64);
            i++;
        }
        ASSERT(tok.kind == Token_EOI);
        ASSERT(i == 6);

        puts("okay");
    }
}


void Compile(view<const char> source, MessageStream *oms);

static bool
CheckStaticAssertFailOnLines(const MessageStream om, uint64_t bits)
{
    for (const Message& m : om) {
        if (m.type == Message_StaticAssertFailed) {
            if (!(bits >> m.line) & 1)
                return false;
            bits &= ~(uint64_t(1) << m.line);
        }
        else {
            return false;
        }
    }

    return bits == 0;
}

int TestSimpleNoCode()
{
    MessageStream om;

    int ncf = 0; // num cases failed

    struct Test {
        int *_pCaseFailCount;
        const char *_caseName;
        bool passed;

        Test(int *pCaseFailCount, const char *name, view<const char> source, MessageStream *pOm)
            : _pCaseFailCount(pCaseFailCount)
            , _caseName(name)
            , passed(true)
        {
            printf("Test: %s\n", name);
            pOm->clear();
            Compile(source, pOm);
        }

        ~Test()
        {
            if (!passed) {
                *this->_pCaseFailCount += 1;
                printf("Test: \"%s\" FAILED\n\n", this->_caseName);
            }
            else {
                printf("Test: \"%s\" passed\n\n", this->_caseName);
            }
        }
    };

    {
        Test t(&ncf, "static_assert(0)", R"(void main(){
        static_assert(0);
})"_view, &om);
        t.passed = CheckStaticAssertFailOnLines(om, 1 << 2);
    }

    {
        Test t(&ncf, "static_assert(1)", R"(void main(){
        static_assert(1);
})"_view, &om);
        if (om.size() == 0) {
            t.passed = true;
        }
    }

    {
        Test t(&ncf, "conxtexpr no vars", R"(void main(){
        static_assert(3 + 7 - 3*3 - 1); // fail, == 0, line 2
        static_assert(4 + 7 - 3*3 - 1); // pass, == 1
        static_assert(90 == 10*9); // pass
        static_assert(90 != 10*9); // fail, line 5
        static_assert(91 != 10*9); // pass, line 6
        static_assert(5*-1 + 100 == 95); // pass, line 7
        static_assert(-5*1 + 100 == 95); // pass, line 8
        static_assert(-5*1 + 100 != 95); // fail, line 9
        static_assert(-5*1 + 100 != 91); // pass, line 11
})"_view, &om);
        t.passed = CheckStaticAssertFailOnLines(om, 1 << 2 | 1 << 5 | 1 << 9);
    }

#if 0
    {
        Test t(&ncf, "conxtexpr vars", R"(void main(){
        constexpr int a = 0; // line 2
        constexpr int b = 1;
        constexpr int c = 7;
        constexpr int d = 0;
        static_assert(a); // line 6
        static_assert(b);
        static_assert(c);
        static_assert(d); // line 9
})"_view, &om);
        t.passed = CheckStaticAssertFailOnLines(om, 1 << 6 | 1 << 9);
    }
#endif

    ASSERT(ncf >= 0);
    if (ncf) {
        printf("\n\nTest cases FAILED: %d.\n", ncf);
    }
    else {
        puts("\n\nAll test cases PASSED.");
    }
    return ncf;
}
