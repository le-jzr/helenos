#ifndef __BITS_NATIVE_H_
#define __BITS_NATIVE_H_

#include <_bits/macros.h>

/* A bunch of type aliases HelenOS code uses.
 * They were originally defined as either u/int32_t or u/int64_t,
 * specifically for each architecture, but in practice they were assumed
 * to be identical to u/intptr_t, which happened to work by accident only.
 */

#define ATOMIC_COUNT_MIN __UINTPTR_MIN__
#define ATOMIC_COUNT_MAX __UINTPTR_MAX__

typedef __UINTPTR_TYPE__ pfn_t;
typedef __UINTPTR_TYPE__ ipl_t;
typedef __UINTPTR_TYPE__ sysarg_t;
typedef __INTPTR_TYPE__ native_t;
typedef __UINTPTR_TYPE__ atomic_count_t;
typedef __INTPTR_TYPE__ atomic_signed_t;

#define PRIdn __PRIdPTR__  /**< Format for native_t. */
#define PRIun __PRIuPTR__  /**< Format for sysarg_t. */
#define PRIxn __PRIxPTR__  /**< Format for hexadecimal sysarg_t. */
#define PRIua __PRIuPTR__  /**< Format for atomic_count_t. */

#endif // __BITS_NATIVE_H_
