/*
 * Copyright (c) 2006 Sergey Bondari
 * Copyright (c) 2006 Jakub Jermar
 * Copyright (c) 2011 Jiri Svoboda
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
 * @brief	Userspace ELF module loader.
 *
 * This module allows loading ELF binaries (both executables and
 * shared objects) from VFS. The current implementation allocates
 * anonymous memory, fills it with segment data and then adjusts
 * the memory areas' flags to the final value. In the future,
 * the segments will be mapped directly from the file.
 */

#include <errno.h>
#include <stdio.h>
#include <vfs/vfs.h>
#include <stddef.h>
#include <stdint.h>
#include <align.h>
#include <assert.h>
#include <as.h>
#include <elf/elf.h>
#include <smc.h>
#include <loader/pcb.h>
#include <entry_point.h>
#include <str_error.h>
#include <stdlib.h>
#include <macros.h>

#include <elf/elf_load.h>
#include "elf_debug.h"

#include "elf2.h"

#define DPRINTF(...)

#if 0

/** Process TLS program header.
 *
 * @param elf  Pointer to loader state buffer.
 * @param hdr  TLS program header
 * @param info Place to store TLS info
 */
static void tls_program_header(elf_ld_t *elf, elf_segment_header_t *hdr,
    elf_tls_info_t *info)
{
	info->tdata = (void *)((uint8_t *)hdr->p_vaddr + elf->bias);
	info->tdata_size = hdr->p_filesz;
	info->tbss_size = hdr->p_memsz - hdr->p_filesz;
	info->tls_align = hdr->p_align;
}

/** Process segment header.
 *
 * @param elf   Pointer to loader state buffer.
 * @param entry	Segment header.
 *
 * @return EOK on success, error code otherwise.
 */
static errno_t segment_header(elf_ld_t *elf, elf_segment_header_t *entry)
{
	switch (entry->p_type) {
	case PT_NULL:
	case PT_PHDR:
	case PT_NOTE:
		break;
	case PT_GNU_EH_FRAME:
	case PT_GNU_STACK:
	case PT_GNU_RELRO:
		/* Ignore GNU headers, if present. */
		break;
	case PT_INTERP:
		elf->info->interp =
		    (void *)((uint8_t *)entry->p_vaddr + elf->bias);

		if (entry->p_filesz == 0) {
			DPRINTF("Zero-sized ELF interp string.\n");
			return EINVAL;
		}
		if (elf->info->interp[entry->p_filesz - 1] != '\0') {
			DPRINTF("Unterminated ELF interp string.\n");
			return EINVAL;
		}
		DPRINTF("interpreter: \"%s\"\n", elf->info->interp);
		break;
	case PT_DYNAMIC:
		/* Record pointer to dynamic section into info structure */
		elf->info->dynamic =
		    (void *)((uint8_t *)entry->p_vaddr + elf->bias);
		DPRINTF("dynamic section found at %p\n",
		    (void *)elf->info->dynamic);
		break;
	case 0x70000000:
	case 0x70000001:
	case 0x70000002:
	case 0x70000003:
		// FIXME: Architecture-specific headers.
		/* PT_MIPS_REGINFO, PT_MIPS_ABIFLAGS, PT_ARM_UNWIND, ... */
		break;
	case PT_TLS:
		/* Parse TLS program header */
		tls_program_header(elf, entry, &elf->info->tls);
		DPRINTF("TLS header found at %p\n",
		    (void *)((uint8_t *)entry->p_vaddr + elf->bias));
		break;
	case PT_SHLIB:
	default:
		DPRINTF("Segment p_type %d unknown.\n", entry->p_type);
		return ENOTSUP;
		break;
	}
	return EOK;
}

#endif

/** @}
 */
