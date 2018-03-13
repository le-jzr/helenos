/*
 * Copyright (c) 2007 Michal Kebrt
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
 *  @brief CPU identification.
 */

#include <arch/cache.h>
#include <arch/cpu.h>

#include <cpu.h>
#include <arch.h>
#include <print.h>

#ifdef CONFIG_FPU
#include <arch/fpu_context.h>
#endif

	// TODO
#if 0
/** Enables unaligned access and caching for armv6+ */
void cpu_arch_init(void)
{
	uint32_t control_reg = SCTLR_read();

	dcache_invalidate();
	read_barrier();

	/* Turn off tex remap, RAZ/WI prior to armv7 */
	control_reg &= ~SCTLR_TEX_REMAP_EN_FLAG;
	/* Turn off accessed flag, RAZ/WI prior to armv7 */
	control_reg &= ~(SCTLR_ACCESS_FLAG_EN_FLAG | SCTLR_HW_ACCESS_FLAG_EN_FLAG);

	/* Unaligned access is supported on armv6+ */
#if defined(PROCESSOR_ARCH_armv7_a) | defined(PROCESSOR_ARCH_armv6)
	/* Enable unaligned access, RAZ/WI prior to armv6
	 * switchable on armv6, RAO/WI writes on armv7,
	 * see ARM Architecture Reference Manual ARMv7-A and ARMv7-R edition
	 * L.3.1 (p. 2456) */
	control_reg |= SCTLR_UNALIGNED_EN_FLAG;
	/* Disable alignment checks, this turns unaligned access to undefined,
	 * unless U bit is set. */
	control_reg &= ~SCTLR_ALIGN_CHECK_EN_FLAG;
	/* Enable caching, On arm prior to armv7 there is only one level
	 * of caches. Data cache is coherent.
	 * "This means that the behavior of accesses from the same observer to
	 * different VAs, that are translated to the same PA
	 * with the same memory attributes, is fully coherent."
	 *    ARM Architecture Reference Manual ARMv7-A and ARMv7-R Edition
	 *    B3.11.1 (p. 1383)
	 * We are safe to turn this on. For arm v6 see ch L.6.2 (p. 2469)
	 * L2 Cache for armv7 is enabled by default (i.e. controlled by
	 * this flag).
	 */
	
	control_reg |= SCTLR_CACHE_EN_FLAG;
#endif
#if defined(PROCESSOR_ARCH_armv7_a)
	 /* ICache coherency is elaborated on in barrier.h.
	  * VIPT and PIPT caches need maintenance only on code modify,
	  * so it should be safe for general use.
	  * Enable branch predictors too as they follow the same rules
	  * as ICache and they can be flushed together
	  */
	if ((CTR_read() & CTR_L1I_POLICY_MASK) != CTR_L1I_POLICY_AIVIVT) {
		control_reg |=
		    SCTLR_INST_CACHE_EN_FLAG | SCTLR_BRANCH_PREDICT_EN_FLAG;
	} else {
		control_reg &=
		    ~(SCTLR_INST_CACHE_EN_FLAG | SCTLR_BRANCH_PREDICT_EN_FLAG);
	}
#endif

	SCTLR_write(control_reg);

#ifdef CONFIG_FPU
	fpu_setup();
#endif

#ifdef PROCESSOR_ARCH_armv7_a
	if ((ID_PFR1_read() & ID_PFR1_GEN_TIMER_EXT_MASK) !=
	    ID_PFR1_GEN_TIMER_EXT) {
		PMCR_write(PMCR_read() | PMCR_E_FLAG | PMCR_D_FLAG);
		PMCNTENSET_write(PMCNTENSET_CYCLE_COUNTER_EN_FLAG);
	}
#endif
}
#endif

	// TODO
	#if 0
/** Retrieves processor identification and stores it to #CPU.arch */
void cpu_identify(void)
{
	arch_cpu_identify(&CPU->arch);
}
#endif

	// TODO
	#if 0
/** Prints CPU identification. */
void cpu_print_report(cpu_t *m)
{
	printf("cpu%d: vendor=%s, architecture=%s, part number=%x, "
	    "variant=%x, revision=%x\n",
	    m->id, implementer(m->arch.imp_num),
	    architecture_string(&m->arch), m->arch.prim_part_num,
	    m->arch.variant_num, m->arch.rev_num);
}
	#endif


/** @}
 */
