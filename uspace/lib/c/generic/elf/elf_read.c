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
 */

// TODO: clean up includes

#include "elf2.h"

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
#include <vfs/vfs.h>

#include <elf/elf_load.h>
#include <loader/loader.h>
#include "../private/loader.h"
#include <async.h>
#include "../private/async.h"
#include <ipc/loader.h>
#include "../private/ht_stref.h"
#include "elf_debug.h"
#include <io/kio.h>

#include "../private/stack.h"

DEFINE_STACK_TYPE(_mod_stack, struct elf_info *);
DEFINE_STACK_TYPE(_str_stack, const char *);

#define DPRINTF(...) kio_printf(__VA_ARGS__)
#define DTRACE(...) kio_printf(__VA_ARGS__)

void elf_info_free(void *arg)
{
	struct elf_info *info = arg;
	vfs_put(info->fd);
	free(info->phdr);
	free(info->dyn);
	free(info->dyn_strtab);
	free(info);
}

static errno_t _read_file(int fd, uintptr_t offset, size_t size, void *buf)
{
	if (size == 0)
		return EOK;

	aoff64_t pos = offset;
	size_t nread;
	errno_t rc = vfs_read(fd, &pos, buf, size, &nread);
	if (rc != EOK)
		return rc;

	if (nread != size)
		return EIO;

	return EOK;
}

static size_t _get_file_size(int fd)
{
	/* Read file size */
	vfs_stat_t stat;
	if (vfs_stat(fd, &stat) != EOK)
		return 0;

	return stat.size > SIZE_MAX ? SIZE_MAX : stat.size;
}

/** Determine file offset in an initialization image for a given virtual address.
 *
 * @param name        Structure name (for debugging printouts only).
 * @param vaddr       Virtual address
 * @param[out] size   Number of bytes present in the file past vaddr.
 * @param phdr        Program header array
 * @param phdr_len    Program header array length
 * @return File offset, or 0 if not found.
 */
static uintptr_t _find_file_offset(const char *name, uintptr_t vaddr, size_t *size,
    elf_segment_header_t phdr[], size_t phdr_len)
{
	for (size_t i = 0; i < phdr_len; i++) {
		if (phdr[i].p_type != PT_LOAD)
			continue;

		if (phdr[i].p_vaddr > vaddr)
			continue;

		uintptr_t segment_offset = (vaddr - phdr[i].p_vaddr);

		if (segment_offset >= phdr[i].p_memsz)
			continue;

		DTRACE("Found %s in segment %zd: 0x%zx .. 0x%zx\n",
		    name, i, phdr[i].p_vaddr, phdr[i].p_vaddr + phdr[i].p_memsz);
		DTRACE("%s offset: 0x%zx\n", name, segment_offset);

		if (segment_offset >= phdr[i].p_filesz) {
			// Completely in the zeroed part of the segment.
			*size = 0;
		} else {
			size_t filesz = phdr[i].p_filesz - segment_offset;
			if (filesz < *size)
				*size = filesz;
		}

		return phdr[i].p_offset + segment_offset;
	}

	return 0;
}

static struct elf_info *elf_read_file(int fd, size_t file_size)
{
	DTRACE("elf_load_info()\n");

	struct elf_info *info = NULL;
	elf_dyn_t *dyn = NULL;
	char *strtab = NULL;

	elf_header_t header;
	errno_t rc = _read_file(fd, 0, sizeof(header), &header);
	if (rc != EOK)
		goto fail;

	if (elf_validate_header(&header, file_size) != EOK)
		goto fail;

	DTRACE("ELF header is valid.\n");

	size_t phdr_len = header.e_phnum;
	size_t phdr_size = phdr_len * sizeof(elf_segment_header_t);

	if (phdr_size / sizeof(elf_segment_header_t) != phdr_len)
		goto fail;

	DTRACE("Allocating info structure.\n");

	info = calloc(sizeof(struct elf_info) + phdr_size, 1);
	if (!info)
		goto fail;

	info->phdr_len = phdr_len;
	elf_segment_header_t *phdr = info->phdr;

	DTRACE("Copying program headers.\n");

	// Copy program headers.
	if (_read_file(fd, header.e_phoff, phdr_size, phdr) != EOK)
		goto fail;

	DTRACE("Validating program headers.\n");

	// Validate all program headers.
	for (size_t i = 0; i < phdr_len; i++) {
		if (elf_validate_phdr(i, &phdr[i], file_size) != EOK)
			goto fail;
	}

	DTRACE("Copying dynamic section.\n");

	// Find the DYNAMIC section.
	size_t dyn_len = 0;
	size_t dyn_size = 0;

	for (size_t i = 0; i < phdr_len; i++) {
		if (phdr[i].p_type != PT_DYNAMIC)
			continue;

		dyn_size = phdr[i].p_memsz;
		dyn_len = dyn_size / sizeof(elf_dyn_t);

		dyn = malloc(dyn_size);
		if (!dyn)
			goto fail;

		if (_read_file(fd, phdr[i].p_offset, phdr[i].p_filesz, dyn) != EOK)
			goto fail;

		memset(dyn + phdr[i].p_filesz, 0, phdr[i].p_memsz - phdr[i].p_filesz);
		break;
	}

	for (size_t i = 0; i < dyn_len; i++) {
		if (dyn[i].d_tag == DT_NULL) {
			dyn_len = i;
			break;
		}
	}

	// Find contents of the dynamic section.
	size_t strtab_len = 0;
	uintptr_t strtab_vaddr = 0;

	for (size_t i = 0; i < dyn_len; i++) {
		switch (dyn[i].d_tag) {
		case DT_STRTAB:
			strtab_vaddr = dyn[i].d_un.d_ptr;
			break;
		case DT_STRSZ:
			strtab_len = dyn[i].d_un.d_val;
			break;
		}
	}

	DTRACE("Copying dynamic string table.\n");

	DTRACE("strtab_len = %zd\n", strtab_len);
	DTRACE("strtab_vaddr = 0x%zx .. 0x%zx\n", strtab_vaddr, strtab_vaddr + strtab_len);

	if (strtab_vaddr != 0) {
		// strtab_vaddr is the (unrelocated) address of the string table in
		// the child process, so we have to go through DT_LOAD phdrs and find
		// where it is in the file.

		strtab = malloc(strtab_len);
		if (!strtab)
			goto fail;

		size_t strtab_filesz = strtab_len;
		uintptr_t offset = _find_file_offset("strtab", strtab_vaddr, &strtab_filesz, phdr, phdr_len);

		if (offset == 0) {
			DPRINTF("String table not present in file.\n");
			goto fail;
		}

		if (_read_file(fd, offset, strtab_filesz, strtab) != EOK)
			goto fail;

		memset(strtab + strtab_filesz, 0, strtab_len - strtab_filesz);
		DTRACE("String table copied.\n");

		if (strtab[0] != '\0' || strtab[strtab_len - 1] != '\0') {
			DPRINTF("Invalid string table\n");
			goto fail;
		}
	}

	DTRACE("Collating ELF information.\n");

	info->fd = fd;
	info->file_size = file_size;
	info->header = header;
	info->dyn = dyn;
	info->dyn_len = dyn_len;
	info->dyn_strtab = strtab;
	info->dyn_strtab_len = strtab_len;

	// To be determined later.
	info->bias = 0;
	info->name = NULL;
	return info;

fail:
	if (info)
		free(info);

	if (dyn)
		free(dyn);

	if (strtab)
		free(strtab);

	return NULL;
}

static struct elf_info *elf_read_file_name(const char *name, int libdir_fds[], size_t libdir_fds_len)
{
	DTRACE("elf_read_file(%s)\n", name);

	errno_t rc;
	int fd;

	if (name[0] == '/') {
		/* Absolute path. */
		rc = vfs_lookup_open(name, WALK_REGULAR, MODE_READ, &fd);
		if (rc != EOK)
			return NULL;
	} else {
		/* Just filename, look in every library search directory provided. */
		rc = ENOENT;

		for (size_t i = 0; i < libdir_fds_len; i++) {
			rc = vfs_walk(libdir_fds[i], name, WALK_REGULAR, &fd);
			if (rc == EOK)
				break;
		}

		if (rc != EOK)
			return NULL;
	}

	/* Open library file for reading. */
	rc = vfs_open(fd, MODE_READ);
	if (rc != EOK) {
		vfs_put(fd);
		return NULL;
	}

	struct elf_info *info = elf_read_file(fd, _get_file_size(fd));
	if (!info) {
		vfs_put(fd);
		return NULL;
	}

	info->name = name;
	return info;
}

/** Computes symbol resolution order for a given tree of libraries.
 * This is breadth-first search as per the standard behavior on other systems.
 *
 * @param libs  Library hash table
 * @param root  Root of the dependency tree
 * @param[out] resolution_order      Place to put newly allocated array.
 * @param[out] resolution_order_len  Length of the newly allocated array.
 * @return ENOMEM or EOK
 */
static errno_t _compute_resolution_order(
	hash_table_t *libs, struct elf_info *root,
	struct elf_info ***resolution_order, size_t *resolution_order_len)
{
	_mod_stack_t bfs_list = { };

	/* Use the bias field here as a "visited" flag. */
	root->bias = 1;
	errno_t rc = _mod_stack_push(&bfs_list, root);
	if (rc != EOK) {
		_mod_stack_destroy(&bfs_list, NULL);
		return rc;
	}

	for (size_t processed = 0; processed < bfs_list.stack_len; processed++) {
		struct elf_info *info = bfs_list.array[processed];
		DTRACE("Resolution order: %s\n", info->name);

		// Go through DT_NEEDED entries.
		for (size_t i = 0; i < info->dyn_len; i++) {
			elf_debug_print_dyn(&info->dyn[i], info->dyn_strtab);
			if (info->dyn[i].d_tag != DT_NEEDED)
				continue;

			// Validity checked earlier.
			const char *needed = info->dyn_strtab + info->dyn[i].d_un.d_val;

			struct elf_info *lib_info = ht_stref_get(libs, needed);
			assert(lib_info);
			if (!lib_info->bias) {
				lib_info->bias = 1;
				rc = _mod_stack_push(&bfs_list, lib_info);
				if (rc != EOK) {
					_mod_stack_destroy(&bfs_list, NULL);
					return rc;
				}
			}
		}
	}

	/* Clean up the bias field. */
	for (size_t i = 0; i < bfs_list.stack_len; i++)
		bfs_list.array[i]->bias = 0;

	DTRACE("Finished computing symbol resolution order.\n");

	*resolution_order = bfs_list.array;
	*resolution_order_len = bfs_list.stack_len;
	return EOK;
}

/** Read program headers and dynamic section for a program and all its
 * dependencies. Returns allocated data structures for all modules in
 * initialization order as well as symbol resolution order.
 * Information for the root ELF file is always first in resolution order.
 *
 * @param exec_name
 * @param exec_fd
 * @param[out] init_order
 * @param[out] res_order
 * @param[out] nmodules
 * @return EOK or error code
 */
errno_t elf_read_modules(const char *root_name, int root_fd, elf_head_t ***init_order, elf_head_t ***res_order, size_t *nmodules)
{
	int libfd = -1;
	hash_table_t libs_ht;
	if (!ht_stref_create(&libs_ht))
		return ENOMEM;

	_mod_stack_t init_list = { };

	// For computing initialization order.
	_mod_stack_t enter_stack = { };

	errno_t rc;
	_str_stack_t stack = { };
	rc = _str_stack_push(&stack, root_name ? root_name : "");
	if (rc != EOK)
		goto fail;

	int modules = 0;

	rc = vfs_lookup("/lib", WALK_DIRECTORY, &libfd);
	if (rc != EOK)
		goto fail;

	while (!_str_stack_empty(&stack)) {
		const char *name = _str_stack_pop(&stack);
		if (!name) {
			// NULL in the main stack indicates we finished processing all dependencies
			// of the entry at the top of enter_stack.
			struct elf_info *info = _mod_stack_pop(&enter_stack);
			rc = _mod_stack_push(&init_list, info);
			if (rc != EOK)
				goto fail;
			DTRACE("Finished processing %s.\n", info->name);
			continue;
		}

		// Already processed this one.
		// Note that we could not have avoided inserting duplicates into the stack,
		// as that would make us unable to correctly compute initialization order.
		if (ht_stref_get(&libs_ht, name) != NULL)
			continue;

		DTRACE("Loading '%s'.\n", name);
		struct elf_info *info;
		if (root_name[0] == '\0') {
			info = elf_read_file(root_fd, _get_file_size(root_fd));
		} else {
			info = elf_read_file_name(name, &libfd, 1);
		}

		if (!info) {
			rc = EINVAL;
			goto fail;
		}

		DTRACE("Done loading %s.\n", name);

		bool success = ht_stref_insert(&libs_ht, name, info);
		assert(success);

		rc = _mod_stack_push(&enter_stack, info);
		if (rc != EOK)
			goto fail;

		rc = _str_stack_push(&stack, NULL);
		if (rc != EOK)
			goto fail;

		modules++;

		DTRACE("Listing DT_NEEDED:\n");

		// Go through DT_NEEDED entries.
		for (size_t i = 0; i < info->dyn_len; i++) {
			elf_debug_print_dyn(&info->dyn[i], info->dyn_strtab);

			if (info->dyn[i].d_tag != DT_NEEDED)
				continue;

			uintptr_t strtab_offset = info->dyn[i].d_un.d_val;
			if (strtab_offset >= info->dyn_strtab_len) {
				DPRINTF("Invalid DT_NEEDED entry.\n");
				rc = EINVAL;
				goto fail;
			}

			const char *needed = info->dyn_strtab + strtab_offset;

			DTRACE("DT_NEEDED(\"%s\")\n", needed);
			rc = _str_stack_push(&stack, needed);
			if (rc != EOK)
				goto fail;
		}

		DTRACE("Done listing.\n");
	}

	DTRACE("Loaded %d modules.\n", modules);

	// Next thing we need is the symbol resolution order,
	// i.e. breadth-first order of the dependency tree.
	size_t resolution_order_len;
	rc = _compute_resolution_order(&libs_ht, init_list.array[init_list.stack_len - 1], res_order, &resolution_order_len);
	if (rc != EOK)
		goto fail;

	assert(resolution_order_len == init_list.stack_len);

	*init_order = init_list.array;
	*nmodules = init_list.stack_len;

	// Now that we're done, free the helper structures.
	// The elf_info structures are now referenced through the two lists.
	_str_stack_destroy(&stack, NULL);
	_mod_stack_destroy(&enter_stack, NULL);
	ht_stref_destroy(&libs_ht, NULL);
	vfs_put(libfd);
	return EOK;

fail:
	if (libfd != -1)
		vfs_put(libfd);
	// Free everything, including elf_info structures in the hash table.
	_mod_stack_destroy(&init_list, NULL);
	_str_stack_destroy(&stack, NULL);
	_mod_stack_destroy(&enter_stack, NULL);
	ht_stref_destroy(&libs_ht, elf_info_free);
	return rc;
}

static size_t _get_load_bounds(elf_head_t *mod)
{
	uintptr_t top = 0;

	for (size_t p = 0; p < mod->phdr_len; p++) {
		elf_segment_header_t *phdr = &mod->phdr[p];
		if (phdr->p_type == PT_LOAD && phdr->p_memsz > 0)
			top = max(top, phdr->p_vaddr + phdr->p_memsz);
	}

	return ALIGN_UP(top, PAGE_SIZE);
}

static errno_t _set_bias(elf_head_t *mod)
{
	if (mod->header.e_type == ET_EXEC) {
		mod->bias = 0;
		return EOK;
	}

	if (mod->header.e_type != ET_DYN)
		return EINVAL;

	size_t module_size = _get_load_bounds(mod);

	/*
	 * Attempt to allocate a span of memory large enough for the
	 * shared object.
	 */
	// FIXME: This is not reliable when we're running
	//        multi-threaded. Even if this part succeeds, later
	//        allocation can fail because another thread took the
	//        space in the meantime. This is only relevant for
	//        dlopen() though.
	void *area = as_area_create(AS_AREA_ANY, module_size,
	    AS_AREA_READ | AS_AREA_WRITE | AS_AREA_CACHEABLE |
	    AS_AREA_LATE_RESERVE, AS_AREA_UNPAGED);

	if (area == AS_MAP_FAILED) {
		DPRINTF("Can't find suitable memory area.\n");
		return ENOMEM;
	}

	mod->bias = (uintptr_t) area;
	as_area_destroy(area);

	return EOK;
}

static unsigned _as_area_flags(elf_segment_header_t *phdr)
{
	unsigned flags = AS_AREA_CACHEABLE;
	if (phdr->p_flags & PF_X)
		flags |= AS_AREA_EXEC;
	if (phdr->p_flags & PF_W)
		flags |= AS_AREA_WRITE;
	if (phdr->p_flags & PF_R)
		flags |= AS_AREA_READ;
	return flags;
}

errno_t elf_load_modules(elf_head_t *modules[], size_t modules_len)
{
	for (size_t i = 0; i < modules_len; i++) {
		elf_head_t *mod = modules[i];

		errno_t rc = _set_bias(mod);
		if (rc != EOK)
			return rc;

		for (size_t p = 0; p < mod->phdr_len; p++) {
			elf_segment_header_t *phdr = &mod->phdr[p];
			if (phdr->p_type != PT_LOAD)
				continue;

			elf_debug_print_segment(p, phdr);

			uintptr_t real_vaddr = phdr->p_vaddr + mod->bias;

			uintptr_t area_base = ALIGN_DOWN(real_vaddr, PAGE_SIZE);
			uintptr_t area_size = ALIGN_UP(phdr->p_memsz + (real_vaddr - area_base), PAGE_SIZE);

			void *area = as_area_create((void *) area_base,
				area_size, AS_AREA_READ | AS_AREA_WRITE | AS_AREA_CACHEABLE,
				AS_AREA_UNPAGED);
			if (area == AS_MAP_FAILED) {
				DPRINTF("Memory mapping failed (%p, %zu)\n", (void *) area_base, area_size);
				return ENOMEM;
			}

			/* Load segment data */
			rc = _read_file(mod->fd, phdr->p_offset, phdr->p_filesz, (void *) real_vaddr);
			if (rc != EOK) {
				DPRINTF("Read error: %s\n", str_error(rc));
				return EIO;
			}

			/* Change to desired permissions. */
			rc = as_area_change_flags(area, _as_area_flags(phdr));
			if (rc != EOK) {
				DPRINTF("Failed to set area flags: %s.\n", str_error(rc));
				elf_debug_print_flags(phdr->p_flags);
				return rc;
			}

			if (phdr->p_flags & PF_X) {
				/* Enforce SMC coherence for the segment */
				if (smc_coherence((void *) area_base, area_size))
					return ENOMEM;
			}
		}
	}

	return EOK;
}

/** @}
 */
