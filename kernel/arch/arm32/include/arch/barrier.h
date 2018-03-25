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
#include <arch/cache.h>
#include <arch/cp15.h>
#include <align.h>
#else
#include <libarch/cp15.h>
#endif

#define CS_ENTER_BARRIER()  asm volatile ("" ::: "memory")
#define CS_LEAVE_BARRIER()  asm volatile ("" ::: "memory")

/* ARMv7 uses instructions for memory barriers see ARM Architecture reference
 * manual for details:
 * DMB: ch. A8.8.43 page A8-376
 * DSB: ch. A8.8.44 page A8-378
 * See ch. A3.8.3 page A3-148 for details about memory barrier implementation
 * and functionality on armv7 architecture.
 */
/*
 * ARMv6 introduced user access of the following commands:
 * - Prefetch flush
 * - Data synchronization barrier
 * - Data memory barrier
 * - Clean and prefetch range operations.
 * ARM Architecture Reference Manual version I ch. B.3.2.1 p. B3-4
 */
/* ARMv6- use system control coprocessor (CP15) for memory barrier instructions.
 * Although at least mcr p15, 0, r0, c7, c10, 4 is mentioned in earlier archs,
 * CP15 implementation is mandatory only for armv6+.
 */
/* Older manuals mention syscalls as a way to implement cache coherency and
 * barriers. See for example ARM Architecture Reference Manual Version D
 * chapter 2.7.4 Prefetching and self-modifying code (p. A2-28)
 */

static inline void dmb(void)
{
#if defined(PROCESSOR_ARCH_armv7_a)
	asm volatile ("dmb" ::: "memory");
#elif defined(PROCESSOR_ARCH_armv6) || defined(KERNEL)
	CP15DMB_write(0);
#else
	// FIXME
#endif
}

static inline void dsb(void)
{
#if defined(PROCESSOR_ARCH_armv7_a)
	asm volatile ("dsb" ::: "memory");
#elif defined(PROCESSOR_ARCH_armv6) || defined(KERNEL)
	CP15DSB_write(0);
#else
	// FIXME
#endif
}

static inline void isb(void)
{
#if defined(PROCESSOR_ARCH_armv7_a)
	asm volatile ("isb" ::: "memory");
#elif defined(PROCESSOR_ARCH_armv6) || defined(KERNEL)
	CP15ISB_write(0);
#else
	// FIXME
#endif
}

#define memory_barrier()  dsb()
#define read_barrier()    dsb()
#define write_barrier()   dsb()
#define inst_barrier()    { dsb(); isb(); }

#ifdef KERNEL

/*
 * There are multiple ways ICache can be implemented on ARM machines. Namely
 * PIPT, VIPT, and ASID and VMID tagged VIVT (see ARM Architecture Reference
 * Manual B3.11.2 (p. 1383).  However, CortexA8 Manual states: "For maximum
 * compatibility across processors, ARM recommends that operating systems target
 * the ARMv7 base architecture that uses ASID-tagged VIVT instruction caches,
 * and do not assume the presence of the IVIPT extension. Software that relies
 * on the IVIPT extension might fail in an unpredictable way on an ARMv7
 * implementation that does not include the IVIPT extension." (7.2.6 p. 245).
 * Only PIPT invalidates cache for all VA aliases if one block is invalidated.
 *
 * @note: Supporting ASID and VMID tagged VIVT may need to add ICache
 * maintenance to other places than just smc.
 */

static inline void _smc_coherence(uintptr_t a)
{
	dcache_clean_mva_pou(ALIGN_DOWN(a, CP15_C7_MVA_ALIGN));
	/* Wait for completion */
	dsb();
	icache_invalidate();
	/* Wait for Inst refetch */
	dsb();
	isb();
}

#define smc_coherence(a) _smc_coherence((uintptr_t)(a))

/* @note: Cache type register is not available in uspace. We would need
 * to export the cache line value, or use syscall for uspace smc_coherence */
static inline void _smc_coherence_block(uintptr_t a, size_t l)
{
	for (uintptr_t addr = a; addr < a + l; addr += CP15_C7_MVA_ALIGN) {
		smc_coherence(addr);
	}
}

#define smc_coherence_block(a, l) _smc_coherence_block((uintptr_t)(a), (l))

#endif /* KERNEL */

#endif

/** @}
 */
