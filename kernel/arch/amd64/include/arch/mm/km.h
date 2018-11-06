/*
 * Copyright (c) 2011 Jakub Jermar
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup kernel_amd64_mm
 * @{
 */
/** @file
 */

#ifndef KERN_amd64_KM_H_
#define KERN_amd64_KM_H_

#include <stdbool.h>
#include <typedefs.h>

#ifdef MEMORY_MODEL_kernel

#define KM_AMD64_IDENTITY_START      UINT64_C(0xffffffff80000000)
#define KM_AMD64_IDENTITY_SIZE       UINT64_C(0x0000000080000000)

/* Shadow memory requires 1/8 of the kernel address space. */
#define KM_SHADOW_START        UINT64_C(0xffff800000000000)
#define KM_SHADOW_SIZE         UINT64_C(0x0000100000000000)

#define KM_AMD64_NON_IDENTITY_START  UINT64_C(0xffff900000000000)
#define KM_AMD64_NON_IDENTITY_SIZE   UINT64_C(0x00006fff80000000)

_Static_assert(
    KM_SHADOW_START + KM_SHADOW_SIZE == KM_AMD64_NON_IDENTITY_START,
    "Non-identity memory doesn't start at the end of shadow memory."
);

_Static_assert(
    KM_AMD64_NON_IDENTITY_START + KM_AMD64_NON_IDENTITY_SIZE ==
        KM_AMD64_IDENTITY_START,
    "Identity memory doesn't start at the end of non-identity memory."
);

#endif /* MEMORY_MODEL_kernel */

#ifdef MEMORY_MODEL_large

#define KM_AMD64_IDENTITY_START      UINT64_C(0xffff800000000000)
#define KM_AMD64_IDENTITY_SIZE       UINT64_C(0x0000400000000000)

#define KM_AMD64_NON_IDENTITY_START  UINT64_C(0xffffc00000000000)
#define KM_AMD64_NON_IDENTITY_SIZE   UINT64_C(0x0000400000000000)

#endif /* MEMORY_MODEL_large */

extern void km_identity_arch_init(void);
extern void km_non_identity_arch_init(void);
extern bool km_is_non_identity_arch(uintptr_t);

#endif

/** @}
 */
