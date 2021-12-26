#pragma once

#include <stddef.h>

void* AllocateBytes(size_t nbytes) noexcept;
void* AllocateZeroedBytes(size_t nbytes) noexcept;
void* ReallocateBytes(void *p, size_t nbytes) noexcept;
void Deallocate(const void *p) noexcept;

template<class T> T* Allocate(size_t n) { return (T *) AllocateBytes(n * sizeof(T)); }
template<class T> T* AllocateZeroed(size_t n) { return (T *) AllocateZeroedBytes(n * sizeof(T)); }
template<class T> T* Reallocate(void *p, size_t n) { return (T *) ReallocateBytes(p, n * sizeof(T)); }
