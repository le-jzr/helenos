/*
 * Copyright (c) 2016 Jiri Svoboda
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

/** @addtogroup libc
 * @{
 */

/**
 * @file
 * @brief	Userspace ELF loader.
 */

#include <elf/elf_load.h>
#include <elf/elf_mod.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <vfs/vfs.h>
#include <macros.h>

#include "elf2.h"

#ifdef CONFIG_RTLD
#include <rtld/rtld.h>
#endif

#define DPRINTF(...)

static void *_exec_base(const elf_head_t *elf)
{
	uintptr_t head = UINTPTR_MAX;

	for (size_t i = 0; i < elf->phdr_len; i++) {
		if (elf->phdr[i].p_type == PT_LOAD)
			head = min(head, elf->phdr[i].p_vaddr);
	}

	return (void *)(head + elf->bias);
}

static void *_exec_dynamic(const elf_head_t *elf)
{
	for (size_t i = 0; i < elf->phdr_len; i++) {
		if (elf->phdr[i].p_type == PT_DYNAMIC)
			return (void *) (elf->phdr[i].p_vaddr + elf->bias);
	}

	return NULL;
}

static void _exec_tls(const elf_head_t *elf, elf_tls_info_t *tls)
{
	*tls = (elf_tls_info_t) { };

	for (size_t i = 0; i < elf->phdr_len; i++) {
		elf_segment_header_t phdr = elf->phdr[i];
		if (phdr.p_type == PT_TLS) {
			tls->tdata = (void *) (phdr.p_vaddr + elf->bias);
			tls->tdata_size = phdr.p_filesz;
			tls->tbss_size = phdr.p_memsz - phdr.p_filesz;
			tls->tls_align = phdr.p_align;
			return;
		}
	}
}

/** Load ELF program.
 *
 * @param file File handle
 * @param info Place to store ELF program information
 * @return EOK on success or an error code
 */
errno_t elf_load(int file, elf_info_t *info)
{
	elf_head_t **init_order;
	elf_head_t **res_order;
	size_t nmodules;

	/* Read module information */
	errno_t rc = elf_read_modules(NULL, file, &init_order, &res_order, &nmodules);
	if (rc != EOK) {
		DPRINTF("Reading modules failed.\n");
		return rc;
	}

	rc = elf_load_modules(res_order, nmodules);
	if (rc != EOK) {
		DPRINTF("Loading modules failed.\n");
		return rc;
	}

	info->finfo.base = _exec_base(res_order[0]);
	info->finfo.dynamic = _exec_dynamic(res_order[0]);
	info->finfo.entry = (void *) (res_order[0]->header.e_entry + res_order[0]->bias);
	_exec_tls(res_order[0], &info->finfo.tls);

	if (res_order[0]->dyn_len == 0) {
		/* Statically linked program */
		DPRINTF("Binary is statically linked.\n");
		info->env = NULL;
		return EOK;
	}

	DPRINTF("Binary is dynamically linked.\n");
#ifdef CONFIG_RTLD
	DPRINTF("- prog dynamic: %p\n", info->finfo.dynamic);

	return rtld_prog_process(&info->finfo, &info->env);
#else
	return ENOTSUP;
#endif
}

/** Set ELF-related PCB entries.
 *
 * Fills the program control block @a pcb with information from
 * @a info.
 *
 * @param info	Program info structure
 * @param pcb PCB
 */
void elf_set_pcb(elf_info_t *info, pcb_t *pcb)
{
	pcb->entry = info->finfo.entry;
	pcb->dynamic = info->finfo.dynamic;
	pcb->rtld_runtime = info->env;
}

/** @}
 */
