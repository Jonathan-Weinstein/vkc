#include <stdio.h>
#include <stdlib.h>

#include "common.h"

// #include "Array.h"
// template class Array<uint32_t>;

noreturn_void
NotImplementedImpl(const char *file, int line, const char *info)
{
    printf("\nNotImplemented at %s:%d, %s.\n", file, line, info);
#ifdef _DEBUG
    __debugbreak();
#else
    exit(42);
#endif
}


void Scanner_TestRaw();
int TestSimpleNoCode();

int main()
{
    Scanner_TestRaw();
    puts("\n\n");
    TestSimpleNoCode();


#if 0
    {
        Array<uint32_t> code;
        ASSERT(code.is_empty());
        code.push(5);

        const uint32_t tmp[] = { 10, 11, 12, 13, 14, 8 };

        code.push_n(tmp, lengthof(tmp));
        code.push_n(tmp, lengthof(tmp));
        printf("cap = %d\n", code.capacity());

        for (uint x : code) {
            printf("x = %d\n", x);
        }
        code.set_size(2);
        
        puts("\n\n");

        for (uint x : code) {
            printf("x = %d\n", x);
        }
        code.clear();
        ASSERT(code.is_empty());
        code.push(7);
        ASSERT(code.pop() == 7);
        ASSERT(code.is_empty());

        printf("cap = %d\n", code.capacity());
    }
#endif

    return 0;
}
