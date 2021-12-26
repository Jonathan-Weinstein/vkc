#pragma once

#include "common.h"
 
/*
 * This "descriptor" is like an opaque handle to a type.
 * It may contain info directly in some handle bits, or a pointer/index to extra data.
 * Layout is subject to change, use getters.
 */
enum TypeDescriptor : uint32_t { NullTypeDesc };
static_assert(uint64_t(BuiltinType_none) == uint64_t(NullTypeDesc), "");

/*
bitstruct TypeDescriptor {
    control : 2;
    union { // off = 2
        bitstruct {
            builtin : 6
            unsigned : 1 // off = 8
            reference : 1 // off = 9
            readonly : 1 // off = 10
        } leaf;
    };
};
*/
enum : unsigned {
    TypeDescControl_Leaf      = 0,
    TypeDescControl_Array     = 1,
    TypeDescControl_Pointer   = 2,
    TypeDescControl_Aggregate = 3, // struct or union
};

typedef uint32_t TypeDescLeafFlags;
enum : TypeDescLeafFlags {
    TypeDescLeafFlag_Unsigned  = 1u << 8,
    TypeDescLeafFlag_Reference = 1u << 9,
    TypeDescLeafFlag_Readonly  = 1u <<10,
};

inline bool IsLeaf(TypeDescriptor td) { return (td & 0x3) == 0; }

#define MaxUintN(n) (~0u >> (32 - (n)))

/* NOTE: These must only be called when IsLeaf is true: */
inline BuiltinTypeKind  LeafBuiltin(TypeDescriptor td)    { ASSERT((td & 0x3) == 0); return BuiltinTypeKind(td >> 2 & MaxUintN(6)); }
inline bool             LeafIsUnsigned(TypeDescriptor td) { ASSERT((td & 0x3) == 0); return (td & TypeDescLeafFlag_Unsigned) != 0; }
inline bool             LeafIsReference(TypeDescriptor td){ ASSERT((td & 0x3) == 0); return (td & TypeDescLeafFlag_Reference) != 0; }
inline bool             LeafIsReadonly(TypeDescriptor td) { ASSERT((td & 0x3) == 0); return (td & TypeDescLeafFlag_Readonly) != 0; }

inline TypeDescriptor MakeLeafTypeDesc(BuiltinTypeKind builtin, TypeDescLeafFlags leafFlags) { return TypeDescriptor(uint32_t(builtin) << 2 | leafFlags); }

// haven't really looked at these much, could be interesting:
// https://github.com/pervognsen/bitwise/blob/master/ion/type.c
//
// https://github.com/ziglang/zig/blob/master/src/type.zig
// https://github.com/ziglang/zig/blob/master/src/Sema.zig

// https://clang.llvm.org/docs/InternalsManual.html#the-type-class-and-its-subclasses
// https://clang.llvm.org/docs/InternalsManual.html#the-qualtype-class
