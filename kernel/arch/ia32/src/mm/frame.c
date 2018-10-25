/*
 * Copyright (c) 2008 Jakub Jermar
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

/** @addtogroup kernel_ia32_mm
 * @{
 */
/** @file
 * @ingroup kernel_ia32_mm, kernel_amd64_mm
 */

#include <mm/frame.h>
#include <arch/mm/frame.h>
#include <mm/as.h>
#include <config.h>
#include <arch/boot/boot.h>
#include <arch/boot/memmap.h>
#include <panic.h>
#include <debug.h>
#include <align.h>
#include <macros.h>
#include <stdio.h>

#define PHYSMEM_LIMIT32  UINT64_C(0x100000000)

static void init_e820_memory(pfn_t minconf, bool low)
{
	unsigned int i;

	for (i = 0; i < e820counter; i++) {
		uint64_t base64 = e820table[i].base_address;
		uint64_t size64 = e820table[i].size;

#ifdef KARCH_ia32
		/*
		 * Restrict the e820 table entries to 32-bits.
		 */
		if (base64 >= PHYSMEM_LIMIT32)
			continue;

		if (base64 + size64 > PHYSMEM_LIMIT32)
			size64 = PHYSMEM_LIMIT32 - base64;
#endif

		uintptr_t base = (uintptr_t) base64;
		size_t size = (size_t) size64;

		if (!frame_adjust_zone_bounds(low, &base, &size))
			continue;

		if (e820table[i].type == MEMMAP_MEMORY_AVAILABLE) {
			/* To be safe, make the available zone possibly smaller */
			uint64_t new_base = ALIGN_UP(base, FRAME_SIZE);
			uint64_t new_size = ALIGN_DOWN(size - (new_base - base),
			    FRAME_SIZE);

			size_t count = SIZE2FRAMES(new_size);
			pfn_t pfn = ADDR2PFN(new_base);
			pfn_t conf;

			if (low) {
				if ((minconf < pfn) || (minconf >= pfn + count))
					conf = pfn;
				else
					conf = minconf;
				zone_create(pfn, count, conf,
				    ZONE_AVAILABLE | ZONE_LOWMEM);
			} else {
				conf = zone_external_conf_alloc(count);
				if (conf != 0)
					zone_create(pfn, count, conf,
					    ZONE_AVAILABLE | ZONE_HIGHMEM);
			}
		} else if ((e820table[i].type == MEMMAP_MEMORY_ACPI) ||
		    (e820table[i].type == MEMMAP_MEMORY_NVS)) {
			/* To be safe, make the firmware zone possibly larger */
			uint64_t new_base = ALIGN_DOWN(base, FRAME_SIZE);
			uint64_t new_size = ALIGN_UP(size + (base - new_base),
			    FRAME_SIZE);

			zone_create(ADDR2PFN(new_base), SIZE2FRAMES(new_size), 0,
			    ZONE_FIRMWARE);
		} else {
			/* To be safe, make the reserved zone possibly larger */
			uint64_t new_base = ALIGN_DOWN(base, FRAME_SIZE);
			uint64_t new_size = ALIGN_UP(size + (base - new_base),
			    FRAME_SIZE);

			zone_create(ADDR2PFN(new_base), SIZE2FRAMES(new_size), 0,
			    ZONE_RESERVED);
		}
	}
}

static const char *e820names[] = {
	"invalid",
	"available",
	"reserved",
	"acpi",
	"nvs",
	"unusable"
};

void physmem_print(void)
{
	unsigned int i;
	printf("[base            ] [size            ] [name   ]\n");

	for (i = 0; i < e820counter; i++) {
		const char *name;

		if (e820table[i].type <= MEMMAP_MEMORY_UNUSABLE)
			name = e820names[e820table[i].type];
		else
			name = "invalid";

		printf("%#018" PRIx64 " %#018" PRIx64 " %s\n", e820table[i].base_address,
		    e820table[i].size, name);
	}
}

static void reserve_shtab(elf_section_header_t *sht, size_t sht_len)
{
	/*
	 * Reserve memory occupied by ELF sections.
	 * The bootloader will load all sections, regarless of whether they are
	 * covered by PT_LOAD or not. We use these extra sections (e.g. symbol
	 * table and debuginfo sections).
	 * Extra care must be taken since these memory spans need not be
	 * frame-aligned.
	 */

	if (sht_len == 0) {
		printf("Error: no section header table available.");
		return;
	}

	uintptr_t bottom = ALIGN_DOWN(KA2PA(sht), FRAME_SIZE);
	size_t sht_size = sht_len * sizeof(sht[0]);
	uintptr_t top = ALIGN_UP(KA2PA(sht) + sht_size, FRAME_SIZE);

	/* Reserve the table itself. */
	frame_mark_unavailable(bottom >> FRAME_WIDTH,
	    (top - bottom) >> FRAME_WIDTH);

	/* Reserve each loaded section. */
	for (size_t i = 0; i < sht_len; i++) {
		if (sht[i].sh_type == SHT_NULL ||
		    sht[i].sh_addr == 0 ||
		    sht[i].sh_size == 0)
			continue;

		bottom = ALIGN_DOWN(sht[i].sh_addr, FRAME_SIZE);
		top = ALIGN_UP(sht[i].sh_addr + sht[i].sh_size, FRAME_SIZE);

		if ((intptr_t) bottom < 0) {
			bottom = KA2PA(bottom);
			top = KA2PA(top);
		}

		printf("Section %zu, %p, %zu\n", i, (void *) bottom, top - bottom);

		frame_mark_unavailable(bottom >> FRAME_WIDTH,
		    (top - bottom) >> FRAME_WIDTH);
	}
}

void frame_low_arch_init(void)
{
	if (config.cpu_active != 1)
		return;

	pfn_t minconf = 1;

#ifdef CONFIG_SMP
	// FIXME: What is the purpose of minconf? Can we remove it?
	uintptr_t ap_end = ALIGN_UP((uintptr_t) ap_bootstrap_end, FRAME_SIZE);
	minconf = max(minconf, ADDR2PFN(ap_end));
#endif

	init_e820_memory(minconf, true);

	/* Reserve frame 0 (BIOS data) */
	frame_mark_unavailable(0, 1);

	reserve_shtab(shtab, shtab_len);
}

void frame_high_arch_init(void)
{
	if (config.cpu_active == 1)
		init_e820_memory(0, false);
}

/** @}
 */
