#pragma once

#include "common.h"

#include "default_alloc.h"

#include <string.h> // memcpy


// An attempt to make larger code non-templated.
struct VoidArray {
    void *pBegin;
    void *pEnd;
    void *pCap;
};

#define BytePtrSub(a, b) (reinterpret_cast<char *>(a) - reinterpret_cast<char *>(b))

inline char * // returns new pEnd
ArrayRealloc(VoidArray& a, size_t newCapInBytes)
{
    ASSERT(newCapInBytes);

    size_t const origSizeInBytes = BytePtrSub(a.pEnd, a.pBegin);

    char *const pBytes = static_cast<char *>(ReallocateBytes(a.pBegin, newCapInBytes));
    char *const pNewEnd = pBytes + origSizeInBytes;
    a.pBegin = pBytes;
    a.pEnd = pNewEnd;
    a.pCap = pBytes + newCapInBytes;
    return pNewEnd;
}

inline char *
ArrayGrowForAppend(VoidArray& a, uint additionalTs, uint sizeOfT)
{
    uint const oldFilledBytes = uint(BytePtrSub(a.pEnd, a.pBegin));
    uint const minCapBytes    = oldFilledBytes + additionalTs * sizeOfT;
    uint const oldFilledTs    = oldFilledBytes / sizeOfT;
    ASSERT(oldFilledBytes < minCapBytes);
    return ArrayRealloc(a, Max<uint>(((oldFilledTs * 3u) / 2u) * sizeOfT, minCapBytes));
}

#if 0
inline char * // returns new pEnd
ArrayEnsureRoomForAppend(ArrayBase& a, uint additionalTs, uint sizeOfT) 
{
    char *pEnd = a.pEnd;
    size_t const nByteRoom = a.pCap - pEnd;
    size_t const additionalBytes = additionalTs * sizeOfT;
    if (nByteRoom < additionalBytes) {
        pEnd = ArrayGrowByAtLeast(a, additionalTs, sizeOfT);
    }
    return pEnd;
}
#endif

template<typename T>
class Array {
    static_assert(__is_trivial(T), "T must be trivial");

    VoidArray a;

#define BEGIN reinterpret_cast<T *>(a.pBegin)
#define END reinterpret_cast<T *>(a.pEnd)
#define CAP reinterpret_cast<T *>(a.pCap)

public:
    constexpr Array() : a{} {
    }
    ~Array() {
        Deallocate(a.pBegin);
    }

    Array(const Array&) = delete;
    Array& operator=(const Array&) = delete;
    
    T *data() const { return BEGIN; }
    T *begin() const { return BEGIN; }
    T *end() const { return END; }
    uint capacity() const { return uint(CAP - BEGIN); }
    uint size() const { return uint(END - BEGIN); }
    bool is_empty() const { return a.pBegin == a.pEnd; }

    T& operator[](size_t i) { ASSERT(i < size_t(END - BEGIN)); return BEGIN[i]; }

    T& pop() // no dtor, so has non-void return:
    {
        ASSERT(BEGIN < END); 

        T *pBack = END;
        a.pEnd = --pBack;
        return *pBack;
    }

    T *set_size(size_t n)
    {
        ASSERT(size_t(CAP - BEGIN) >= n);
        T *newEnd = BEGIN + n;
        a.pEnd = newEnd;
        return newEnd;
    }

    void set_end(T *p) // illegal to pass in a ptr invalidated by a potentialy reallocating function
    {
        ASSERT(p <= CAP);
        a.pEnd = p;
    }

    void clear()
    {
        a.pEnd = a.pBegin;
    }

    void reserve(uint minCapacity)
    {
        uint minCapacityBytes = minCapacity * sizeof(T);
        if (uint(BytePtrSub(a.pCap, a.pBegin)) < minCapacityBytes) {
            ArrayRealloc(a, minCapacityBytes);
        }
    }

    T* uninitialized_push()
    {
        T *oldEnd = END;
        if (oldEnd == CAP) { // unlikely
            oldEnd = reinterpret_cast<T *>(ArrayGrowForAppend(a, 1, sizeof(T)));
        }
        a.pEnd = reinterpret_cast<char *>(oldEnd + 1);
        return oldEnd;
    }

    T* uninitialized_push_n(uint n)
    {
        T *oldEnd = END;
        if (CAP - END < n) { // unlikely
            oldEnd = reinterpret_cast<T *>(ArrayGrowForAppend(a, n, sizeof(T)));
        }
        a.pEnd = oldEnd + n;
        return oldEnd;
    }

    void push(const T& val)
    {
        *uninitialized_push() = val;
    }

    void push_n(const T *src, uint n)
    {
        memcpy(uninitialized_push_n(n), src, n * sizeof(T));
    }

#undef BEGIN
#undef END
#undef CAP
};
