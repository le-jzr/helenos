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

#define DPRINTF(...) kio_printf(__VA_ARGS__)
#define DTRACE(...) kio_printf(__VA_ARGS__)

void elf_info_free(void *arg)
{
	struct elf_info *info = arg;
	vfs_put(info->fd);
	free(info->phdr);
	free(info->dyn);
	free(info->strtab);
	free(info);
}

typedef struct stack {
	void **array;
	size_t array_len;
	size_t stack_len;
} stack_t;

#define STACK_INITIALIZER { .array = NULL, .array_len = 0, .stack_len = 0 }

static void stack_push(stack_t *stack, const void *val)
{
	if (stack->stack_len >= stack->array_len) {
		stack->array_len = 2 * stack->array_len;
		if (stack->array_len == 0)
			stack->array_len = 4;

		void *new_array = realloc(stack->array, stack->array_len * sizeof(void *));
		if (!new_array) {
			fprintf(stderr, "Out of memory.\n");
			exit(1);
		}

		stack->array = new_array;
	}

	stack->array[stack->stack_len] = (void *) val;
	stack->stack_len++;
}

static void *stack_pop(stack_t *stack)
{
	if (stack->stack_len <= 0)
		return NULL;

	stack->stack_len--;
	return stack->array[stack->stack_len];
}

static bool stack_empty(stack_t *stack)
{
	return stack->stack_len == 0;
}

static void stack_destroy(stack_t *stack, void (*destroy_fn)(void *))
{
	if (destroy_fn) {
		while (stack->stack_len > 0)
			destroy_fn(stack_pop(stack));
	}

	free(stack->array);
	stack->array = NULL;
	stack->array_len = 0;
	stack->stack_len = 0;
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

#if 0
static errno_t _load_segment(int fd, uintptr_t file_offset, uintptr_t file_size, uintptr_t mem_size, void *real_vaddr)
{
	void *actual_vaddr = as_area_create(real_vaddr, ALIGN_UP(mem_size, PAGE_SIZE), AS_AREA_READ | AS_AREA_WRITE | AS_AREA_CACHEABLE, AS_AREA_UNPAGED);
	if (actual_vaddr != real_vaddr) {
		DPRINTF("Failed as_area_create(%p, 0x%zx) segment\n", real_vaddr, mem_size);
		return ENOMEM;
	}

	return _read_file(fd, file_offset, file_size, real_vaddr);
}

static errno_t program_load_header(struct elf_info *info, int i)
{
	elf_segment_header_t *phdr = &info->phdr[i];

	if (phdr->p_memsz == 0)
		return EOK;

	// Bias already needs to be aligned properly, so here we just align to page boundaries for mapping.
	assert(phdr->p_align == 0 || info->bias == ALIGN_DOWN(info->bias, phdr->p_align));

	uintptr_t real_vaddr = phdr->p_vaddr + info->bias;

	if (!IS_ALIGNED(real_vaddr, PAGE_SIZE) || !IS_ALIGNED(phdr->p_offset, PAGE_SIZE)) {
		DPRINTF("Misaligned PT_LOAD segment.\n");
		elf_debug_print_segment(i, phdr);
		return EINVAL;
	}

	errno_t rc = _load_segment(info->fd, phdr->p_offset, phdr->p_filesz, phdr->p_memsz, (void *) real_vaddr);
	if (rc != EOK) {
		DPRINTF("Failed loading segment: %s.\n", str_error(rc));
		elf_debug_print_segment(i, phdr);
		return rc;
	}

	return EOK;
}

static errno_t elf_map_modules(struct elf_info *modules[], size_t modules_len)
{
	errno_t rc;

	for (size_t m = 0; m < modules_len; m++) {
		DPRINTF("Mapping module %s\n", modules[m]->name);

		for (size_t i = 0; i < modules[m]->phdr_len; i++) {
			elf_debug_print_segment(i, &modules[m]->phdr[i]);

			if (modules[m]->phdr[i].p_type != PT_LOAD)
				continue;

			rc = program_load_header(modules[m], i);
			if (rc != EOK)
				return rc;
		}
	}

	return EOK;
}

#endif

static char *str_concat(const char *a, const char *b)
{
	size_t len_a = strlen(a);
	size_t len_b = strlen(b);

	char *buffer = malloc(len_a + len_b + 1);
	if (!buffer)
		return NULL;

	memcpy(buffer, a, len_a);
	memcpy(buffer + len_a, b, len_b + 1);
	return buffer;
}

static uintptr_t find_file_offset(const char *name, uintptr_t vaddr, size_t *size,
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

static struct elf_info *elf_load_info(int fd, size_t file_size)
{
	DTRACE("elf_load_info()\n");

	elf_dyn_t *dyn = NULL;
	char *strtab = NULL;
	elf_segment_header_t *phdr = NULL;

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

	DTRACE("Copying program headers.\n");

	// Copy program headers.
	phdr = malloc(phdr_size);
	if (!phdr)
		goto fail;

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
		case DT_SYMTAB:
		case DT_HASH:
			break;
		default:
			DPRINTF("Unknown dynamic section contents\n");
		}
	}

	DTRACE("Copying string table.\n");

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
		uintptr_t offset = find_file_offset("strtab", strtab_vaddr, &strtab_filesz, phdr, phdr_len);

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

	// Find program headers in PT_LOAD segments.
	size_t phdr_offset = header.e_phoff;

	for (size_t i = 0; i < phdr_len; i++) {
		if (phdr[i].p_type != PT_LOAD || phdr[i].p_offset > phdr_offset)
			continue;

		uintptr_t shift = phdr_offset - phdr[i].p_offset;

		if (shift > phdr[i].p_filesz ||
		    phdr_len > (phdr[i].p_filesz - shift) / sizeof(elf_segment_header_t))
			continue;
		break;
	}

	DTRACE("Collating ELF information.\n");

	struct elf_info *info = malloc(sizeof(struct elf_info));
	if (!info)
		goto fail;

	info->fd = fd;
	info->file_size = file_size;
	info->header = header;
	info->phdr = phdr;
	info->phdr_len = phdr_len;
	info->dyn = dyn;
	info->dyn_len = dyn_len;
	info->strtab = strtab;
	info->strtab_len = strtab_len;
	// To be determined later.
	info->bias = 0;
	info->name = NULL;
	info->visited = false;
	return info;

fail:
	free(phdr);
	free(dyn);
	free(strtab);
	return NULL;
}

static struct elf_info *elf_read_file(const char *name)
{
	DTRACE("elf_read_file(%s)\n", name);

	// Load the file.
	char *filename = name[0] == '/' ? strdup(name) : str_concat("/lib/", name);
	if (!filename)
		return NULL;

	int fd;
	errno_t rc = vfs_lookup_open(filename, WALK_REGULAR, MODE_READ, &fd);
	free(filename);
	if (rc != EOK)
		return NULL;

	vfs_stat_t stat;
	if (vfs_stat(fd, &stat) != EOK) {
		vfs_put(fd);
		return NULL;
	}

	struct elf_info *info = elf_load_info(fd, stat.size);

	DTRACE("elf_load_info() exitted\n");

	if (!info)
		return NULL;

	info->name = name;
	return info;
}

static void compute_resolution_order(hash_table_t *libs, struct elf_info *root, stack_t *bfs_list)
{
	root->visited = true;
	stack_push(bfs_list, root);

	for (size_t processed = 0; processed < bfs_list->stack_len; processed++) {
		struct elf_info *info = bfs_list->array[processed];
		DTRACE("Resolution order: %s\n", info->name);

		// Go through DT_NEEDED entries.
		for (size_t i = 0; i < info->dyn_len; i++) {
			elf_debug_print_dyn(&info->dyn[i], info->strtab);
			if (info->dyn[i].d_tag != DT_NEEDED)
				continue;

			// Validity checked earlier.
			const char *needed = info->strtab + info->dyn[i].d_un.d_val;

			struct elf_info *lib_info = ht_stref_get(libs, needed);
			assert(lib_info);
			if (!lib_info->visited) {
				lib_info->visited = true;
				stack_push(bfs_list, lib_info);
			}
		}
	}

	DTRACE("Finished computing symbol resolution order.\n");
}

errno_t elf_read_modules_2(const char *exec_name, elf_head_t ***init_order, elf_head_t ***res_order, size_t *nmodules)
{
	hash_table_t libs_ht;
	if (!ht_stref_create(&libs_ht))
		return ENOMEM;

	stack_t init_list = STACK_INITIALIZER;
	stack_t bfs_list = STACK_INITIALIZER;

	// For computing initialization order.
	stack_t enter_stack = STACK_INITIALIZER;

	errno_t rc;
	stack_t stack = STACK_INITIALIZER;
	stack_push(&stack, exec_name);

	int modules = 0;

	while (!stack_empty(&stack)) {
		const char *name = stack_pop(&stack);
		if (!name) {
			// NULL in the main stack indicates we finished processing all dependencies
			// of the entry at the top of enter_stack.
			struct elf_info *info = stack_pop(&enter_stack);
			stack_push(&init_list, info);
			DTRACE("Finished processing %s.\n", info->name);
			continue;
		}

		// Already processed this one.
		// Note that we could not have avoided inserting duplicates into the stack,
		// as that would make us unable to correctly compute initialization order.
		if (ht_stref_get(&libs_ht, name) != NULL)
			continue;

		DTRACE("Loading %s.\n", name);

		struct elf_info *info = elf_read_file(name);
		if (!info) {
			rc = EINVAL;
			goto fail;
		}

		DTRACE("Done loading %s.\n", name);

		bool success = ht_stref_insert(&libs_ht, name, info);
		assert(success);

		stack_push(&enter_stack, info);
		stack_push(&stack, NULL);
		modules++;

		DTRACE("Listing DT_NEEDED:\n");

		// Go through DT_NEEDED entries.
		for (size_t i = 0; i < info->dyn_len; i++) {
			elf_debug_print_dyn(&info->dyn[i], info->strtab);

			if (info->dyn[i].d_tag != DT_NEEDED)
				continue;

			uintptr_t strtab_offset = info->dyn[i].d_un.d_val;
			if (strtab_offset >= info->strtab_len) {
				DPRINTF("Invalid DT_NEEDED entry.\n");
				rc = EINVAL;
				goto fail;
			}

			const char *needed = info->strtab + strtab_offset;

			DTRACE("DT_NEEDED(\"%s\")\n", needed);
			stack_push(&stack, needed);
		}

		DTRACE("Done listing.\n");
	}

	DTRACE("Loaded %d modules.\n", modules);

	// Next thing we need is the symbol resolution order,
	// i.e. breadth-first order of the dependency tree.
	compute_resolution_order(&libs_ht, init_list.array[init_list.stack_len - 1], &bfs_list);

	assert(init_list.stack_len == bfs_list.stack_len);

	*init_order = (elf_head_t **) init_list.array;
	*res_order = (elf_head_t **) bfs_list.array;
	*nmodules = init_list.stack_len;

	// Now that we're done, free the helper structures.
	// The elf_info structures are now referenced through the two lists.
	stack_destroy(&stack, NULL);
	stack_destroy(&enter_stack, NULL);
	ht_stref_destroy(&libs_ht, NULL);
	return EOK;

fail:
	// Free everything, including elf_info structures in the hash table.
	stack_destroy(&init_list, NULL);
	stack_destroy(&bfs_list, NULL);
	stack_destroy(&stack, NULL);
	stack_destroy(&enter_stack, NULL);
	ht_stref_destroy(&libs_ht, elf_info_free);
	return rc;
}

void elf_compute_bias(uintptr_t *vaddr_limit, struct elf_info *infos[], size_t infos_len)
{
	for (size_t i = 0; i < infos_len; i++) {
		struct elf_info *info = infos[i];

		bool pic = (info->header.e_type == ET_DYN);
		uintptr_t start = UINTPTR_MAX;
		uintptr_t end = 0;
		uintptr_t align = PAGE_SIZE;

		for (size_t p = 0; p < info->phdr_len; p++) {
			const elf_segment_header_t *phdr = &info->phdr[p];

			if (phdr->p_type != PT_LOAD)
				continue;

			if (align < phdr->p_align)
				align = phdr->p_align;

			if (start > phdr->p_vaddr)
				start = phdr->p_vaddr;

			if (end < phdr->p_vaddr + phdr->p_memsz)
				end = phdr->p_vaddr + phdr->p_memsz;
		}

		DPRINTF("%s module %s: 0x%zx .. 0x%zx (size = 0x%zx, align = 0x%zx)\n",
		    pic ? "PIC" : "Fixed-position", info->name, start, end, end - start, align);

		if (end <= start) {
			continue;
		}

		if (!pic) {
			info->bias = 0;
			continue;
		}

		// Set bias to the highest value we can go while
		// staying under vaddr_limit and aligned as requested.
		info->bias = ALIGN_DOWN(*vaddr_limit - end, align);

		DPRINTF("PIC module %s bias set to 0x%zx: 0x%zx .. 0x%zx\n",
		    info->name, info->bias, info->bias + start, info->bias + end);

		// Don't know why anyone would make a position-independent binary
		// starting at a large vaddr, but we handle it gracefully anyway.
		*vaddr_limit = ALIGN_DOWN(info->bias + start, PAGE_SIZE);
	}
}

#if 0
errno_t elf_load_file_name2(const char *path, const char *cwd, const char *const args[],
    int fd_stdin, int fd_stdout, int fd_stderr)
{
	stack_t init_list = STACK_INITIALIZER;
	stack_t bfs_list = STACK_INITIALIZER;

	errno_t rc = elf_read_modules_2(path, &init_list, &bfs_list);
	if (rc != EOK)
		return rc;

	DTRACE("Modules read.\n");

	// Determine load address for each position-independent module.
	uintptr_t vaddr_limit = sys_vaddr_limit();
	compute_bias(&vaddr_limit, (struct elf_info **) init_list.array, init_list.stack_len);
	DPRINTF("vaddr_limit after placing PIC modules: 0x%zx\n", vaddr_limit);

	elf_map_modules(child, (struct elf_info **) init_list.array, init_list.stack_len);

	*out_task = child;

exit:
	stack_destroy(&bfs_list, NULL);
	stack_destroy(&init_list, elf_info_free);
	return rc;
}
#endif

/** @}
 */
