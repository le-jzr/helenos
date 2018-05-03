#ifndef C_H_
#define C_H_

// FIXME

// Miscelaneous definitions.

#include <abi/errno.h>
#include <_bits/errno.h>
#include <_bits/size_t.h>
#include <_bits/NULL.h>
#include <_bits/stdint.h>
#include <_bits/inttypes.h>

void *aligned_alloc(size_t, size_t);
void *calloc(size_t, size_t);
void free(void *);
void *malloc(size_t);
void *realloc(void *, size_t);
int snprintf(char *__restrict, size_t, const char *__restrict, ...);
void *memcpy(void *__restrict, const void *__restrict, size_t);
void *memmove(void *, const void *, size_t);
void *memset(void *, int, size_t);

// Used instead of assigning `errno` directly.
// Kernel doesn't have thread-locals, so we allow this to be a noop.
void __set_errno(errno_t);

#endif
