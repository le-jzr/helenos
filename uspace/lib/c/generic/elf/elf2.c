/*
 * Copyright (c) 2006 Sergey Bondari
 * Copyright (c) 2006 Jakub Jermar
 * Copyright (c) 2011 Jiri Svoboda
 * Copyright (c) 2022 Jiří Zárevúcky
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
#include <stdalign.h>
#include <string.h>
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
#include "../private/sys.h"

#define DPRINTF(...) printf(__VA_ARGS__)


static void debug_print_segment_type(elf_word type)
{
	DPRINTF("    p_type: ");

	switch (type) {
	case PT_NULL:
		DPRINTF("PT_NULL");
		break;
	case PT_PHDR:
		DPRINTF("PT_PHDR");
		break;
	case PT_NOTE:
		DPRINTF("PT_NOTE");
		break;
	case PT_INTERP:
		DPRINTF("PT_INTERP");
		break;
	case PT_DYNAMIC:
		DPRINTF("PT_DYNAMIC");
		break;
	case PT_TLS:
		DPRINTF("PT_TLS");
		break;
	case PT_SHLIB:
		DPRINTF("PT_SHLIB");
		break;
	case PT_GNU_EH_FRAME:
		DPRINTF("PT_GNU_EH_FRAME");
		break;
	case PT_GNU_STACK:
		DPRINTF("PT_GNU_STACK");
		break;
	case PT_GNU_RELRO:
		DPRINTF("PT_GNU_RELRO");
		break;
	default:
		DPRINTF("0x%x", type);
		break;
	}

	DPRINTF("\n");
}

static void debug_print_flags(elf_word flags)
{
	DPRINTF("    p_flags:");
	if (flags & PF_R)
		DPRINTF(" PF_R");
	if (flags & PF_W)
		DPRINTF(" PF_W");
	if (flags & PF_X)
		DPRINTF(" PF_X");
	flags &= ~(PF_R | PF_W | PF_X);
	if (flags)
		DPRINTF(" 0x%x", flags);
	DPRINTF("\n");
}

static void debug_print_segment(int i, const elf_segment_header_t *phdr)
{
	DPRINTF("Segment %d {\n", i);
	debug_print_segment_type(phdr->p_type);
	debug_print_flags(phdr->p_flags);
	DPRINTF("    p_offset: 0x%llx (%llu)\n", (unsigned long long) phdr->p_offset, (unsigned long long) phdr->p_offset);
	DPRINTF("    p_vaddr: 0x%llx (%llu)\n", (unsigned long long) phdr->p_vaddr, (unsigned long long) phdr->p_vaddr);
	DPRINTF("    p_paddr: 0x%llx (%llu)\n", (unsigned long long) phdr->p_paddr, (unsigned long long) phdr->p_paddr);
	DPRINTF("    p_filesz: 0x%llx (%llu)\n", (unsigned long long) phdr->p_filesz, (unsigned long long) phdr->p_filesz);
	DPRINTF("    p_memsz: 0x%llx (%llu)\n", (unsigned long long) phdr->p_memsz, (unsigned long long) phdr->p_memsz);
	DPRINTF("    p_align: 0x%llx (%llu)\n", (unsigned long long) phdr->p_align, (unsigned long long) phdr->p_align);
	DPRINTF("}\n");
}

static errno_t validate_phdr(int i, const elf_segment_header_t *phdr, uint64_t elf_size)
{
	if ((phdr->p_flags & ~(PF_X | PF_R | PF_W)) != 0) {
		DPRINTF("Unknown flags in segment header.\n");
		debug_print_segment(i, phdr);
		return EINVAL;
	}

	uint64_t offset = phdr->p_offset;
	uint64_t filesz = phdr->p_filesz;
	uint64_t page_limit = UINTPTR_MAX - PAGE_SIZE + 1;

	if (elf_size < offset || elf_size < filesz) {
		DPRINTF("Truncated ELF file, file size = 0x%llx (%llu).\n", (unsigned long long) elf_size, (unsigned long long) elf_size);
		debug_print_segment(i, phdr);
		return EINVAL;
	}

	// Check that ALIGN_UP(offset + filesz, PAGE_SIZE) doesn't overflow.
	if (offset > page_limit || filesz > page_limit - offset) {
		DPRINTF("Declared segment file size too large.\n");
		debug_print_segment(i, phdr);
		return EINVAL;
	}

	// Check that file data is in bounds, even after aligning segment boundaries to multiples of PAGE_SIZE.
	if (elf_size < ALIGN_UP(offset + filesz, PAGE_SIZE)) {
		DPRINTF("Truncated ELF file, file size = 0x%llx (%llu).\n", (unsigned long long) elf_size, (unsigned long long) elf_size);
		debug_print_segment(i, phdr);
		return EINVAL;
	}

	uint64_t vaddr = phdr->p_vaddr;
	uint64_t memsz = phdr->p_memsz;
	uint64_t max = UINTPTR_MAX;

	if (memsz > 0) {
		if (memsz > max || vaddr > max || (max - (memsz - 1)) < vaddr) {
			DPRINTF("vaddr + memsz is outside legal memory range.\n");
			debug_print_segment(i, phdr);
			return EINVAL;
		}

		if (vaddr < PAGE_SIZE && vaddr + memsz > UINTPTR_MAX - PAGE_SIZE + 1) {
			// After alignment, segment spans the entire address space,
			// so its real size overflows uintptr_t.
			DPRINTF("Segment spans entire address space.\n");
			debug_print_segment(i, phdr);
			return EINVAL;
		}
	}

	if (phdr->p_memsz < phdr->p_filesz) {
		DPRINTF("memsz < filesz\n");
		debug_print_segment(i, phdr);
		return EINVAL;
	}

	if (!(phdr->p_flags & PF_R) && phdr->p_filesz != 0) {
		DPRINTF("Nonzero p_filesz in a segment with no read permission.\n");
		debug_print_segment(i, phdr);
		return EINVAL;
	}

	if (!(phdr->p_flags & PF_W) && (phdr->p_filesz != phdr->p_memsz) && (phdr->p_offset + phdr->p_filesz) % PAGE_SIZE != 0) {
		// Technically could be supported, but it's more likely a linking bug than an intended feature.
		DPRINTF("File data does not end on a page boundary (would need zeroing out of page end) in a non-writable segment.\n");
		debug_print_segment(i, phdr);
		return EINVAL;
	}

	size_t align = max(PAGE_SIZE, phdr->p_align);

	// Check that alignment is a power of two.
	if (__builtin_popcount(align) != 1) {
		DPRINTF("non power-of-2 alignment\n");
		debug_print_segment(i, phdr);
		return EINVAL;
	}

	if (phdr->p_vaddr % align != phdr->p_offset % align) {
		DPRINTF("vaddr is misaligned with offset\n");
		debug_print_segment(i, phdr);
		return EINVAL;
	}

	return EOK;
}

static errno_t program_load_header(task_handle_t child, int i, const elf_segment_header_t *phdr, const void *elf_base, uint64_t elf_size, mem_handle_t mem)
{
	if (phdr->p_memsz == 0)
		return EOK;

	int flags = AS_AREA_CACHEABLE;
	if (phdr->p_flags & PF_R)
		flags |= AS_AREA_READ;
	if (phdr->p_flags & PF_W)
		flags |= AS_AREA_WRITE;
	if (phdr->p_flags & PF_X)
		flags |= AS_AREA_EXEC;

	// True alignment will become relevant later for deciding offset of position-independent code,
	// but for mapping we rely on the linker giving us a properly aligned segments.
	uintptr_t page_vaddr = ALIGN_DOWN(phdr->p_vaddr, PAGE_SIZE);
	uint64_t page_offset = ALIGN_DOWN(phdr->p_offset, PAGE_SIZE);
	assert(page_vaddr - phdr->p_vaddr == page_offset - phdr->p_offset);
	uint64_t page_file_size_unaligned = (page_offset - phdr->p_offset) + phdr->p_filesz;
	uint64_t page_file_size = ALIGN_UP(page_file_size_unaligned, PAGE_SIZE);
	// Make sure this works right even for the case where the segment touches the very top of the address space.
	uintptr_t page_mem_size = ALIGN_UP((page_vaddr - phdr->p_vaddr) + phdr->p_memsz, PAGE_SIZE);

	assert(page_mem_size >= page_file_size);
	assert(elf_size - page_offset >= page_file_size);
	assert(page_offset + page_file_size_unaligned == phdr->p_offset + phdr->p_filesz);

	if (phdr->p_filesz > 0) {
		// We don't write-map the original memory image. Instead it's mapped as copy-on-write.
		if (flags & AS_AREA_WRITE)
			flags |= AS_AREA_COW;

		if (sys_task_mem_map(child, mem, page_offset, page_file_size, page_vaddr, flags) == MEM_MAP_FAILED) {
			DPRINTF("Overlapping segments.\n");
			debug_print_segment(i, phdr);
			return EINVAL;
		}

		if (phdr->p_memsz > phdr->p_filesz && page_file_size > page_file_size_unaligned) {
			assert(page_file_size % PAGE_SIZE == 0);
			assert(page_file_size > 0);

			// We got extra bits belonging to another segment in the last page.
			// That means we have to manually clear them.

			// We reject non-writable segments with this feature during validation.
			// If lifting this restriction is desired, you need to allocate the last page
			// separately as a writable mem, map it locally (writable without COW), fill it in
			// manually, unmap it, and then map it into the child with the correct flags.
			assert(flags & AS_AREA_WRITE);

			// Zero out the extra bits.
			uintptr_t start = page_vaddr + page_file_size_unaligned;
			uintptr_t end = page_vaddr + page_file_size;

			errno_t rc = sys_task_mem_set(child, start, 0, start - end);
			// If the earlier mem_map succeeded, this cannot fail.
			assert(rc == EOK);
		}
	}

	// The rest of the segment is just zeroes.
	if (page_mem_size > page_file_size) {
		// MEM_NULL means these pages are allocated on demand when written (if writable).
		// For non-writable segments, while pointless, also works just fine
		// and maps just one global immutable zero page to all of it.
		if (sys_task_mem_map(child, MEM_NULL, 0, page_mem_size - page_file_size, page_vaddr + page_file_size, flags) == MEM_MAP_FAILED) {
			DPRINTF("Overlapping segments.\n");
			debug_print_segment(i, phdr);
			return EINVAL;
		}
	}

	return EOK;
}

static errno_t program_header(task_handle_t child, int i, const elf_segment_header_t *phdr, const void *elf_base, uint64_t elf_size, mem_handle_t mem)
{
	switch (phdr->p_type) {
	case PT_LOAD:
		return program_load_header(child, i, phdr, elf_base, elf_size, mem);

	case PT_NULL:
	case PT_NOTE:
	case PT_PHDR:
	case PT_TLS:
		return EOK;

	case PT_GNU_EH_FRAME:
	case PT_GNU_STACK:
	case PT_GNU_RELRO:
		/* Ignore GNU headers, if present. */
		return EOK;

	case PT_DYNAMIC:
	case PT_INTERP:
	case PT_SHLIB:
	default:
		DPRINTF("unsupported program header\n");
		return EINVAL;
	}
}

static void debug_print_elf_header(const elf_header_t *header)
{
	DPRINTF("TODO: print ELF header\n");
}

static errno_t validate_elf_header(const elf_header_t *header, uint64_t elf_size)
{
	/* Identify ELF */
	if (header->e_ident[EI_MAG0] != ELFMAG0 ||
	    header->e_ident[EI_MAG1] != ELFMAG1 ||
	    header->e_ident[EI_MAG2] != ELFMAG2 ||
	    header->e_ident[EI_MAG3] != ELFMAG3) {
		DPRINTF("Invalid magic numbers in ELF file header.\n");
		debug_print_elf_header(header);
		return EINVAL;
	}

	/* Identify ELF compatibility */
	if (header->e_ident[EI_DATA] != ELF_DATA_ENCODING ||
	    header->e_machine != ELF_MACHINE ||
	    header->e_ident[EI_VERSION] != EV_CURRENT ||
	    header->e_version != EV_CURRENT ||
	    header->e_ident[EI_CLASS] != ELF_CLASS) {
		DPRINTF("Incompatible data/version/class.\n");
		debug_print_elf_header(header);
		return EINVAL;
	}

	if (header->e_phentsize != sizeof(elf_segment_header_t)) {
		DPRINTF("e_phentsize: %u != %zu\n", header->e_phentsize,
		    sizeof(elf_segment_header_t));
		debug_print_elf_header(header);
		return EINVAL;
	}

	/* Check if the object type is supported. */
	if (header->e_type != ET_EXEC && header->e_type != ET_DYN) {
		DPRINTF("Object type %d is not supported\n", header->e_type);
		debug_print_elf_header(header);
		return EINVAL;
	}

	if (header->e_phoff == 0) {
		DPRINTF("Program header table is not present!\n");
		debug_print_elf_header(header);
		return EINVAL;
	}

	// Check that all of program header table is inside the file.
	if (header->e_phoff >= elf_size) {
		DPRINTF("Truncated ELF file, file size = 0x%"PRIx64" (%"PRIu64")\n", elf_size, elf_size);
		debug_print_elf_header(header);
		return EINVAL;
	}

	if ((elf_size - header->e_phoff) / header->e_phentsize < header->e_phnum) {
		DPRINTF("Truncated ELF file, file size = 0x%"PRIx64" (%"PRIu64")\n", elf_size, elf_size);
		debug_print_elf_header(header);
		return EINVAL;
	}

	// Check alignment.
	if (header->e_phoff % alignof(elf_segment_header_t) != 0) {
		DPRINTF("Program header table has invalid alignment.");
		debug_print_elf_header(header);
		return EINVAL;
	}

	return EOK;
}


static errno_t elf_spawn_task(const char *name, const void *elf_base, uint64_t elf_size, mem_handle_t mem, task_handle_t *out_task)
{
	const elf_header_t *header = elf_base;

	errno_t rc = validate_elf_header(header, elf_size);
	if (rc != EOK)
		return rc;

	const elf_segment_header_t *phdrs = elf_base + header->e_phoff;

	for (int i = 0; i < header->e_phnum; i++) {
		rc = validate_phdr(i, &phdrs[i], elf_size);
		if (rc != EOK)
			return rc;
	}

	task_handle_t child = sys_task_create(name);
	if (!child)
		return ENOMEM;

	for (int i = 0; i < header->e_phnum; i++) {
		debug_print_segment(i, &phdrs[i]);
		rc = program_header(child, i, &phdrs[i], elf_base, elf_size, mem);
		if (rc != EOK) {
			sys_kobj_put(child);
			return rc;
		}
	}

	// TODO: create stack and start thread

	DPRINTF("Done.\n");
	*out_task = child;
	return EOK;
}

/** Load ELF binary from a file.
 *
 * Load an ELF binary from the specified file. If the file is
 * an executable program, it is loaded unbiased. If it is a shared
 * object, it is loaded with the bias @a so_bias. Some information
 * extracted from the binary is stored in a elf_info_t structure
 * pointed to by @a info.
 *
 * @param file      ELF file.
 * @param info      Pointer to a structure for storing information
 *                  extracted from the binary.
 *
 * @return EOK on success or an error code.
 *
 */
errno_t elf_load_file2(const char *name, int file, task_handle_t *out_task)
{
	vfs_stat_t stat = {0};
	errno_t rc;
	int ofile = -1;
	mem_handle_t mem = MEM_NULL;
	void *vaddr = AS_MAP_FAILED;

	rc = vfs_clone(file, -1, true, &ofile);
	if (rc != EOK)
		goto fail;

	rc = vfs_open(ofile, MODE_READ);
	if (rc != EOK)
		goto fail;

	rc = vfs_stat(ofile, &stat);
	if (rc != EOK)
		goto fail;

	size_t size = ALIGN_UP(stat.size, PAGE_SIZE);

	rc = ENOMEM;

	mem = sys_mem_create(size, AS_AREA_READ|AS_AREA_WRITE|AS_AREA_CACHEABLE);
	if (mem == MEM_NULL)
		goto fail;

	vaddr = sys_mem_map(mem, 0, size, AS_AREA_ANY, AS_AREA_READ|AS_AREA_WRITE|AS_AREA_CACHEABLE);
	if (vaddr == AS_MAP_FAILED)
		goto fail;

	aoff64_t offset = 0;
	size_t nread = 0;

	rc = vfs_read(ofile, &offset, vaddr, stat.size, &nread);
	vfs_put(ofile);
	ofile = -1;

	if (rc != EOK)
		goto fail;

	if (nread != stat.size) {
		DPRINTF("Read less than initially determined file size.\n");
		rc = EINVAL;
		goto fail;
	}

	// Turn the newly loaded memory area read-only.
	rc = sys_mem_remap(vaddr, size, AS_AREA_READ|AS_AREA_CACHEABLE);
	assert(rc == EOK);
	rc = sys_mem_change_flags(mem, AS_AREA_READ|AS_AREA_CACHEABLE);
	assert(rc == EOK);

	rc = elf_spawn_task(name, vaddr, nread, mem, out_task);
	if (rc != EOK)
		goto fail;

	return EOK;

fail:
	if (ofile != -1)
		vfs_put(ofile);

	sys_kobj_put(mem);

	if (vaddr != AS_MAP_FAILED)
		sys_mem_unmap(vaddr, size);

	return rc;
}

errno_t elf_load_file_name2(const char *path, task_handle_t *out_task)
{
	int file;
	errno_t rc = vfs_lookup(path, 0, &file);
	if (rc == EOK) {
		rc = elf_load_file2(path, file, out_task);
		vfs_put(file);
		return rc;
	} else {
		return EINVAL;
	}
}

/** @}
 */
