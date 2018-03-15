/*
 * Copyright (c) 2005 Jakub Jermar
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

/** @addtogroup arm32
 * @{
 */
/** @file
 *  @brief Memory barriers.
 */

#ifndef KERN_arm32_BARRIER_H_
#define KERN_arm32_BARRIER_H_

#ifdef KERNEL
#include <arch/sysarm.h>
#else
#include <libarch/sysarm.h>
#endif

#define CS_ENTER_BARRIER()  asm volatile ("" ::: "memory")
#define CS_LEAVE_BARRIER()  asm volatile ("" ::: "memory")

#define memory_barrier() sysarm_memory_barrier()
#define read_barrier() sysarm_read_memory_barrier()
#define write_barrier() sysarm_write_memory_barrier()

#define smc_coherence_block(a, l) { \
	uintptr_t _a = (uintptr_t) a; \
	sysarm_flush_modified_code_range(_a, _a + l); \
}

#define smc_coherence(a) smc_coherence_block(a, 4)

#endif

/** @}
 */
