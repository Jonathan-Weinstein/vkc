#pragma once

#include <stddef.h>
#include <stdint.h>

#if defined _MSC_VER
	#define unreachable __assume(0)
	#define forceinline __forceinline
	#define noreturn_void __declspec(noreturn) void
#elif defined __GNUC__
	#define unreachable __builtin_unreachable()
	#define forceinline __attribute__((always_inline))
    #define noreturn_void __attribute__((noreturn)) void
#else
	#define unreachable ((void)0)
	#define forceinline
    #define noreturn_void void
#endif

#if defined DEBUG || defined _DEBUG
    #ifdef NDEBUG
        #error "NDEBUG defined the same time as (_DEBUG || DEBUG)."
    #endif
	#define is_debug 1
#else
	#define is_debug 0
#endif

#if is_debug
	#ifdef NDEBUG
		#error "is_debug is nonzero, but NDEBUG is defined."
	#endif

	#include <assert.h>
	#define ASSERT assert
#else
	#define ASSERT(e) ((void)0)
#endif

typedef unsigned uint;
typedef unsigned char ubyte;

template<class T, unsigned N> char(&_lo_helper(const T(&)[N]))[N];
#define lengthof(a) unsigned(sizeof _lo_helper(a))
template<class T, unsigned N> constexpr T* endof(T(&a)[N]) noexcept { return a + N; }

/*  Differences between Min/Man and std::min/std::max:
    - Parameters are by value instead of by const&.
	- Max() returns what Min() would not; Max() returns "b" when one value isn't greater than the other. */
template<class T> T Min(T a, T b) noexcept { return b < a ? b : a; } // float: minss b, a; minss a, b probably better in most circumstances.
template<class T> T Max(T a, T b) noexcept { return b < a ? a : b; } // float: maxss a, b


template<class T>
struct view {
	T *ptr;
	uint length;

	// ranged for:
	T* begin() const { return ptr; }
	T* end() const { return ptr+length; }
};

// The count on input does not include the '\0' terminator:
inline constexpr view<const char> operator""_view(const char *strlit, size_t n) noexcept
{
    return { strlit, uint(n) };
}

noreturn_void NotImplementedImpl(const char *file, int line, const char *info);
#define NotImplemented(info) NotImplementedImpl(__FILE__, __LINE__, info)


enum BuiltinTypeKind : uint8_t {
    BuiltinType_none        =  0,
    BuiltinType_void        =  1,
    BuiltinType_bool        =  2,
    BuiltinType_g8          =  3,
    BuiltinType_g16         =  4,
    BuiltinType_g32         =  5,
    BuiltinType_g64         =  6,
    BuiltinType_fp16        =  7,
    BuiltinType_fp32        =  8,
    BuiltinType_fp64        =  9
#define BuiltinType_EnumEnd   10
};


enum LexErrorKind : uint8_t {
	LexError_InvalidByte,
	LexError_IntLiteralOver64Bits,
	LexError_NameTooLong,
    LexError_BlockCommentNoEnd,   // Got "/*", but hit EOI before a "*/". 
    LexError_BlockCommentNoBegin, // Got "*/", without a previous "/*"
};

typedef unsigned SpvId;

enum SpvValueId : SpvId { NullValueId };
enum SpvTypeId : SpvId { NullTypeId };
