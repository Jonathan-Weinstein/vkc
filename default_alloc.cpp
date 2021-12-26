#include "common.h" // ASSERT

#include <stdlib.h>
#include <stdio.h> // perror

enum : uint32_t { MaxAlloc = 1u<<30 }; // 1 GB

void *
AllocateBytes(size_t nbytes) noexcept
{
	ASSERT(nbytes <= MaxAlloc);
	void *const m = malloc(nbytes);
	if (!m) {
		perror("malloc");
		exit(1);
	}
	return m;
}

void *
ReallocateBytes(void *p, size_t nbytes) noexcept
{
	ASSERT(nbytes <= MaxAlloc);
	void *const m = realloc(p, nbytes);
	if (!m) {
		perror("realloc");
		exit(1);
	}
	return m;
}

void *
AllocateZeroedBytes(size_t nbytes) noexcept
{
	ASSERT(nbytes <= MaxAlloc);
	void *const m = calloc(nbytes, 1);
	if (!m) {
		perror("calloc");
		exit(1);
	}
	return m;
}

void
Deallocate(const void *p) noexcept
{
	free(const_cast<void *>(p));
}


noreturn_void Todo(const char *s)
{
    printf("TODO: %s\n", s);
    exit(1);
}
