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

// In this structure, we keep a copy of the file header,
// program headers, and the dynamic section, so that we don't
// need to keep all the files filling up our virtual
// address space. We also copy them onto child's stack
// next to argv if they aren't already part of any DT_LOAD
// segment.
//
struct elf_info {
	mem_handle_t mem;
	size_t file_size;
	elf_header_t header;
	elf_segment_header_t *phdr;
	size_t phdr_len;
	elf_dyn_t *dyn;
	size_t dyn_len;
	char *strtab;
	size_t strtab_len;
	uintptr_t reloc_entry_vaddr;
	uintptr_t file_header_vaddr;
	uintptr_t phdr_vaddr;

	uintptr_t bias;
	const char *name;

	bool visited;
	uintptr_t info_vaddr;
};

static void elf_info_free(void *arg)
{
	struct elf_info *info = arg;
	sys_kobj_put(info->mem);
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

static errno_t program_load_header(task_handle_t child, struct elf_info *info, int i)
{
	elf_segment_header_t *phdr = &info->phdr[i];

	if (phdr->p_memsz == 0)
		return EOK;

	int flags = AS_AREA_CACHEABLE;
	if (phdr->p_flags & PF_R)
		flags |= AS_AREA_READ;
	if (phdr->p_flags & PF_W)
		flags |= AS_AREA_WRITE;
	if (phdr->p_flags & PF_X)
		flags |= AS_AREA_EXEC;

	// Bias already needs to be aligned properly, so here we just align to page boundaries for mapping.
	assert(phdr->p_align == 0 || info->bias == ALIGN_DOWN(info->bias, phdr->p_align));

	uintptr_t real_vaddr = phdr->p_vaddr + info->bias;

	uintptr_t page_vaddr = ALIGN_DOWN(real_vaddr, PAGE_SIZE);
	uint64_t page_offset = ALIGN_DOWN(phdr->p_offset, PAGE_SIZE);
	assert(real_vaddr - page_vaddr == phdr->p_offset - page_offset);
	uint64_t page_file_size_unaligned = (phdr->p_offset - page_offset) + phdr->p_filesz;
	uint64_t page_file_size = ALIGN_UP(page_file_size_unaligned, PAGE_SIZE);

	// This must work even for the case where the segment touches the very top of the address space.
	uintptr_t page_mem_size_unaligned = (real_vaddr - page_vaddr) + phdr->p_memsz;
	uintptr_t page_mem_size = ALIGN_UP(page_mem_size_unaligned, PAGE_SIZE);

	assert(page_mem_size >= page_file_size);

	if (phdr->p_filesz > 0) {
		assert(info->file_size - page_offset >= page_file_size);
		assert(page_vaddr + page_mem_size_unaligned == real_vaddr + phdr->p_memsz);
		assert(page_offset + page_file_size_unaligned == phdr->p_offset + phdr->p_filesz);

		// We don't write-map the original memory image. Instead it's mapped as copy-on-write.
		if (flags & AS_AREA_WRITE)
			flags |= AS_AREA_COW;

		if (sys_task_mem_map(child, info->mem, page_offset, page_file_size, &page_vaddr, flags) != EOK) {
			DPRINTF("Overlapping segments.\n");
			elf_debug_print_segment(i, phdr);
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

			errno_t rc = sys_task_mem_set(child, start, 0, end - start);
			// If the earlier mem_map succeeded, this cannot fail.
			assert(rc == EOK);
		}
	}

	// The rest of the segment is just zeroes.
	if (page_mem_size > page_file_size) {
		uintptr_t vaddr = page_vaddr + page_file_size;
		// MEM_NULL means these pages are allocated on demand when written (if writable).
		// For non-writable segments, while pointless, also works just fine
		// and maps just one global immutable zero page to all of it.
		if (sys_task_mem_map(child, MEM_NULL, 0, page_mem_size - page_file_size, &vaddr, flags) != EOK) {
			DPRINTF("Overlapping segments.\n");
			elf_debug_print_segment(i, phdr);
			return EINVAL;
		}
	}

	return EOK;
}

static errno_t elf_map_modules(task_handle_t child, struct elf_info *modules[], size_t modules_len)
{
	errno_t rc;

	for (size_t m = 0; m < modules_len; m++) {
		DPRINTF("Mapping module %s\n", modules[m]->name);

		for (size_t i = 0; i < modules[m]->phdr_len; i++) {
			elf_debug_print_segment(i, &modules[m]->phdr[i]);

			if (modules[m]->phdr[i].p_type != PT_LOAD)
				continue;

			rc = program_load_header(child, modules[m], i);
			if (rc != EOK)
				return rc;
		}
	}

	return EOK;
}

static errno_t open_loader_session(task_handle_t child, loader_t *ldr)
{
	cap_phone_handle_t phone;
	errno_t rc = sys_task_connect(child, &phone);
	if (rc != EOK) {
		DPRINTF("Failed connecting to child task: %s\n", strerror(rc));
		return rc;
	}

	async_sess_t sess;
	sess.iface = 0;
	sess.mgmt = EXCHANGE_ATOMIC;
	sess.phone = phone;
	sess.arg1 = 0;
	sess.arg2 = 0;
	sess.arg3 = 0;

	fibril_mutex_initialize(&sess.remote_state_mtx);
	sess.remote_state_data = NULL;

	list_initialize(&sess.exch_list);
	fibril_mutex_initialize(&sess.mutex);
	sess.exchanges = 0;

	async_exch_t *exch = async_exchange_begin(&sess);
	async_sess_t *sess_real = async_connect_me_to(exch, 0, 0, 0, &rc);
	async_exchange_end(exch);
	if (!sess_real) {
		DPRINTF("Failed reconnecting to child task: %s\n", strerror(rc));
		return rc;
	}

	ldr->sess = sess_real;
	return EOK;
}

static errno_t async_finalize(task_handle_t child, int fd_stdin, int fd_stdout, int fd_stderr)
{
	loader_t ldr;
	errno_t rc = open_loader_session(child, &ldr);
	if (rc != EOK)
		return rc;

	/* Send files */
	int root = vfs_root();
	if (root >= 0) {
		rc = loader_add_inbox(&ldr, "root", root);
		vfs_put(root);
		if (rc != EOK) {
			DPRINTF("Failed sending root file handle: %s\n", strerror(rc));
			return rc;
		}
	}

	if (fd_stdin >= 0) {
		rc = loader_add_inbox(&ldr, "stdin", fd_stdin);
		if (rc != EOK) {
			DPRINTF("Failed sending stdin file handle: %s\n", strerror(rc));
			return rc;
		}
	}

	if (fd_stdout >= 0) {
		rc = loader_add_inbox(&ldr, "stdout", fd_stdout);
		if (rc != EOK) {
			DPRINTF("Failed sending stdout file handle: %s\n", strerror(rc));
			return rc;
		}
	}

	if (fd_stderr >= 0) {
		rc = loader_add_inbox(&ldr, "stderr", fd_stderr);
		if (rc != EOK) {
			DPRINTF("Failed sending stderr file handle: %s\n", strerror(rc));
			return rc;
		}
	}

	async_hangup(ldr.sess);
	return EOK;
}

static errno_t elf_spawn_task(task_handle_t child, const char *name, void **init_order,
		void **resolution_order, size_t module_count,
		const char *const args[], const char *cwd, int fd_stdin, int fd_stdout,
	    int fd_stderr, uintptr_t vaddr_limit)
{
	// Compute storage needed for argv.
	size_t argstr_size = 0;

	int argc = 0;
	for (int i = 0; args[i] != NULL; i++) {
		argc++;
		argstr_size += strlen(args[i]) + 1;
	}

	// Store PCB and other things at the top of addressable space.
	uintptr_t alloc_ptr = vaddr_limit;

#define alloc(type, count) ({ \
	alloc_ptr -= sizeof(type) * (count); \
	alloc_ptr = ALIGN_DOWN(alloc_ptr, alignof(type)); \
	alloc_ptr; \
})

	uintptr_t pcb_base = alloc(pcb_t, 1);
	uintptr_t argv_base = alloc(void *, argc + 1);
	uintptr_t init_list_base = alloc(void *, module_count);
	uintptr_t res_list_base = alloc(void *, module_count);

	for (size_t i = 0; i < module_count; i++) {
		struct elf_info *info = init_order[i];
		info->info_vaddr = alloc(elf_rtld_info_t, 1);
	}

	const size_t cwd_size = strlen(cwd) + 1;
	uintptr_t cwd_base = alloc(char, cwd_size);

	uintptr_t argstr_base = alloc(char, argstr_size);

	// Align to page size.
	alloc_ptr = ALIGN_DOWN(alloc_ptr, PAGE_SIZE);

	uintptr_t stack_size = 16 * PAGE_SIZE;
	uintptr_t stack_base = alloc_ptr - stack_size;

	// FIXME: not portable
	uintptr_t pcb_pointer_base = stack_base + stack_size - sizeof(void *);

	int stack_flags = AS_AREA_READ | AS_AREA_WRITE | AS_AREA_CACHEABLE | AS_AREA_LATE_RESERVE;

	errno_t rc = sys_task_mem_map(child, MEM_NULL, 0, vaddr_limit - stack_base, &stack_base, stack_flags);
	if (rc != EOK) {
		sys_kobj_put(child);
		DPRINTF("Failed mapping child stack: %s\n", strerror(rc));
		return rc;
	}

	sys_task_mem_write(child, cwd_base, cwd, cwd_size);

	for (int i = 0; i < argc; i++) {
		size_t arg_size = strlen(args[i]) + 1;
		uintptr_t arg_loc = argstr_base;
		argstr_base += arg_size;
		sys_task_mem_write(child, arg_loc, args[i], arg_size);
		sys_task_mem_write(child, argv_base + i * sizeof(void *), &arg_loc, sizeof(void *));
	}

	// Null terminator for argv.
	sys_task_mem_set(child, argv_base + argc * sizeof(void *), 0, sizeof(void *));

	// Store information on ELF modules.
	for (size_t i = 0; i < module_count; i++) {
		struct elf_info *info = init_order[i];

		// TODO: don't assume that the ELF headers are part of PT_LOAD segments.
		assert(info->file_header_vaddr != UINTPTR_MAX);
		assert(info->phdr_vaddr != UINTPTR_MAX);

		elf_rtld_info_t rtld = {
			.bias = info->bias,
			.header = info->bias + info->file_header_vaddr,
			.phdr = info->bias + info->phdr_vaddr,
		};

		sys_task_mem_write(child, info->info_vaddr, &rtld, sizeof(rtld));
		sys_task_mem_write(child, init_list_base + i * sizeof(void *), &info->info_vaddr, sizeof(void *));
	}

	uintptr_t reloc_entry_vaddr = 0;

	for (size_t i = 0; i < module_count; i++) {
		struct elf_info *info = resolution_order[i];

		// Find the relocation entry point.
		if (!reloc_entry_vaddr && info->reloc_entry_vaddr)
			reloc_entry_vaddr = info->bias + info->reloc_entry_vaddr;

		sys_task_mem_write(child, res_list_base + i * sizeof(void *), &info->info_vaddr, sizeof(void *));
	}

	DPRINTF("reloc_entry_vaddr = 0x%zx\n", reloc_entry_vaddr);

	pcb_t pcb = {
		.entry = 0,
		.cwd = (void *) cwd_base,
		.argc = argc,
		.argv = (void *) argv_base,
		.inbox = NULL,
		.inbox_entries = 0,
		.dynamic = NULL,
		.rtld_runtime = NULL,
		.tcb = NULL,

		.reloc_entry = (void (*)(pcb_t *)) reloc_entry_vaddr,
		.tls_template = NULL,
		.initialization_order = (void *) init_list_base,
		.resolution_order = (void *) res_list_base,
		.module_count = module_count,

		.vaddr_limit = vaddr_limit,
		.initial_stack_limit = stack_base + stack_size,
		.initial_stack_base = stack_base,
	};

	sys_task_mem_write(child, pcb_base, &pcb, sizeof(pcb));
	sys_task_mem_write(child, pcb_pointer_base, &pcb_base, sizeof(pcb_base));

	struct elf_info *main_info = resolution_order[0];

	rc = sys_task_thread_start(child, "main", main_info->header.e_entry, stack_base, stack_size);
	if (rc != EOK) {
		sys_kobj_put(child);
		DPRINTF("Failed starting child thread: %s\n", strerror(rc));
		return rc;
	}

	rc = async_finalize(child, fd_stdin, fd_stdout, fd_stderr);
	if (rc != EOK) {
		// TODO: kill task
		sys_kobj_put(child);
		return rc;
	}

	DPRINTF("Done.\n");
	return EOK;
}

static errno_t read_file(const char *filename, mem_handle_t *out_handle, void **out_vaddr, size_t *out_size)
{
	DPRINTF("read_file(%s)\n", filename);

	int file = -1;
	errno_t rc = vfs_lookup(filename, 0, &file);
	if (rc != EOK)
		return rc;

	vfs_stat_t stat = {0};
	mem_handle_t mem = MEM_NULL;
	void *vaddr = AS_MAP_FAILED;

	rc = vfs_open(file, MODE_READ);
	if (rc != EOK)
		goto fail;

	rc = vfs_stat(file, &stat);
	if (rc != EOK)
		goto fail;

	size_t size = ALIGN_UP(stat.size, PAGE_SIZE);

	rc = ENOMEM;

	mem = sys_mem_create(size, 0, AS_AREA_READ|AS_AREA_WRITE|AS_AREA_CACHEABLE);
	if (mem == MEM_NULL)
		goto fail;

	vaddr = sys_mem_map(mem, 0, size, AS_AREA_ANY, AS_AREA_READ|AS_AREA_WRITE|AS_AREA_CACHEABLE);
	if (vaddr == AS_MAP_FAILED)
		goto fail;

	aoff64_t offset = 0;
	size_t nread = 0;

	rc = vfs_read(file, &offset, vaddr, stat.size, &nread);
	vfs_put(file);
	file = -1;

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
	rc = sys_mem_change_flags(mem, AS_AREA_READ|AS_AREA_EXEC|AS_AREA_CACHEABLE);
	assert(rc == EOK);

	if (out_handle)
		*out_handle = mem;
	else
		sys_kobj_put(mem);

	if (out_vaddr)
		*out_vaddr = vaddr;
	else
		sys_mem_unmap(vaddr, size);

	if (out_size)
		*out_size = size;

	return EOK;

fail:
	if (file != -1)
		vfs_put(file);

	sys_kobj_put(mem);

	if (vaddr != AS_MAP_FAILED)
		sys_mem_unmap(vaddr, size);

	return rc;
}

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

static unsigned long elf_hash(const char *s)
{
	const unsigned char *p = (const unsigned char *)s;

	// Straight out of the spec.

    unsigned long h = 0, high;
    while (*p) {
        h = (h << 4) + *p++;
        if ((high = h & 0xF0000000))
            h ^= high >> 24;
        h &= ~high;
    }
    return h;
}

static elf_symbol_t *lookup_symbol(uint32_t hash[], size_t hash_len,
		elf_symbol_t symtab[], size_t symtab_len, const char *strtab, size_t strtab_len, const char *symbol_name)
{
	DTRACE("Looking for symbol \"%s\"\n", symbol_name);

	uint32_t nbuckets = hash[0];
	uint32_t bucket = elf_hash(symbol_name) % nbuckets;

	if (bucket + 2 >= hash_len)
		return NULL;

	uint32_t sym_idx = hash[bucket + 2];

	while (sym_idx != STN_UNDEF && sym_idx < symtab_len) {
		elf_symbol_t *sym = &symtab[sym_idx];

		const char *sym_name = (sym->st_name >= strtab_len) ? "" : (strtab + sym->st_name);
		DTRACE("Found symbol \"%s\"\n", sym_name);

		if (elf_hash(sym_name) % nbuckets != bucket) {
			DPRINTF("Symbol \"%s\" in unexpected bucket.\n", sym_name);
		}

		if (strcmp(symbol_name, sym_name) == 0)
			return sym;

		uint32_t chain_idx = nbuckets + 2 + sym_idx;

		if (chain_idx >= hash_len)
			return NULL;

		sym_idx = hash[chain_idx];
	}

	return NULL;
}

static struct elf_info *elf_load_info(mem_handle_t mem, void *vaddr, size_t file_size)
{
	DTRACE("elf_load_info()\n");

	elf_dyn_t *dyn = NULL;
	char *strtab = NULL;
	elf_segment_header_t *phdr = NULL;

	elf_header_t *header = (elf_header_t *) vaddr;
	if (elf_validate_header(header, file_size) != EOK)
		goto fail;

	DTRACE("ELF header is valid.\n");

	size_t phdr_len = header->e_phnum;
	size_t phdr_size = phdr_len * sizeof(elf_segment_header_t);

	if (phdr_size / sizeof(elf_segment_header_t) != phdr_len)
		goto fail;

	DTRACE("Copying program headers.\n");

	// Copy program headers.
	phdr = malloc(phdr_size);
	if (!phdr)
		goto fail;

	memcpy(phdr, vaddr + header->e_phoff, phdr_size);

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

		void *dynp = malloc(dyn_size);
		if (!dynp)
			goto fail;

		memcpy(dynp, vaddr + phdr[i].p_offset, phdr[i].p_filesz);
		memset(dynp + phdr[i].p_filesz, 0, phdr[i].p_memsz - phdr[i].p_filesz);

		dyn = dynp;
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

	uintptr_t symtab_vaddr = 0;
	uintptr_t hash_vaddr = 0;

	uintptr_t reloc_entry_vaddr = 0;

	for (size_t i = 0; i < dyn_len; i++) {
		switch (dyn[i].d_tag) {
		case DT_STRTAB:
			strtab_vaddr = dyn[i].d_un.d_ptr;
			break;
		case DT_STRSZ:
			strtab_len = dyn[i].d_un.d_val;
			break;
		case DT_SYMTAB:
			symtab_vaddr = dyn[i].d_un.d_ptr;
			break;
		case DT_HASH:
			hash_vaddr = dyn[i].d_un.d_ptr;
			break;
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

		memcpy(strtab, vaddr + offset, strtab_filesz);
		memset(strtab + strtab_filesz, 0, strtab_len - strtab_filesz);
		DTRACE("String table copied.\n");

		if (strtab[0] != '\0' || strtab[strtab_len - 1] != '\0') {
			DPRINTF("Invalid string table\n");
			goto fail;
		}
	}

	DTRACE("Inspecting hash and symbol table.\n");

	if (hash_vaddr != 0 && symtab_vaddr != 0) {
		size_t hash_filesz = SIZE_MAX;
		uintptr_t offset = find_file_offset("hash", hash_vaddr, &hash_filesz, phdr, phdr_len);

		if (offset == 0 || hash_filesz < 2 * sizeof(uint32_t)) {
			DPRINTF("Empty symbol table\n");
		} else {
			uint32_t *hash = vaddr + offset;

			size_t hash_len = hash[0] + hash[1] + 2;
			if (hash_len > hash_filesz / sizeof(uint32_t))
				hash_len = hash_filesz / sizeof(uint32_t);

			size_t symtab_len = hash[1];
			size_t symtab_filesz = symtab_len * sizeof(elf_symbol_t);

			offset = find_file_offset("symtab", symtab_vaddr, &symtab_filesz, phdr, phdr_len);
			if (offset != 0 && symtab_filesz != 0) {
				if (symtab_len > symtab_filesz / sizeof(elf_symbol_t))
					symtab_len = symtab_filesz / sizeof(elf_symbol_t);

				elf_symbol_t *symtab = vaddr + offset;

				// Lookup relocator entry point.
				elf_symbol_t *sym = lookup_symbol(hash, hash_len,
						symtab, symtab_len, strtab, strtab_len, RELOCATOR_NAME_STRING);

				if (sym)
					reloc_entry_vaddr = sym->st_value;
			}
		}
	}

	// Find file header in PT_LOAD segments.
	uintptr_t file_header_vaddr = UINTPTR_MAX;

	for (size_t i = 0; i < phdr_len; i++) {
		if (phdr[i].p_type == PT_LOAD
				&& phdr[i].p_offset == 0
				&& phdr[i].p_filesz > sizeof(elf_header_t)) {
			file_header_vaddr = phdr[i].p_vaddr;
			break;
		}
	}

	// Find program headers in PT_LOAD segments.
	uintptr_t phdr_vaddr = UINTPTR_MAX;
	size_t phdr_offset = header->e_phoff;

	for (size_t i = 0; i < phdr_len; i++) {
		if (phdr[i].p_type != PT_LOAD || phdr[i].p_offset > phdr_offset)
			continue;

		uintptr_t shift = phdr_offset - phdr[i].p_offset;

		if (shift > phdr[i].p_filesz
				|| phdr_len > (phdr[i].p_filesz - shift) / sizeof(elf_segment_header_t))
			continue;

		phdr_vaddr = phdr[i].p_vaddr + shift;
		break;
	}


	DTRACE("Collating ELF information.\n");

	struct elf_info *info = malloc(sizeof(struct elf_info));
	if (!info)
		goto fail;

	info->mem = mem;
	info->file_size = file_size;
	info->header = *header;
	info->phdr = phdr;
	info->phdr_len = phdr_len;
	info->dyn = dyn;
	info->dyn_len = dyn_len;
	info->strtab = strtab;
	info->strtab_len = strtab_len;
	info->reloc_entry_vaddr = reloc_entry_vaddr;
	info->file_header_vaddr = file_header_vaddr;
	info->phdr_vaddr = phdr_vaddr;
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

	mem_handle_t mem = MEM_NULL;
	void *vaddr = NULL;
	size_t file_size = 0;

	// Load the file.
	char *filename = name[0] == '/' ? strdup(name) : str_concat("/lib/", name);
	if (!filename)
		return NULL;

	errno_t rc = read_file(filename, &mem, &vaddr, &file_size);
	free(filename);
	if (rc != EOK)
		return NULL;

	struct elf_info *info = elf_load_info(mem, vaddr, file_size);

	DTRACE("elf_load_info() exitted\n");

	sys_mem_unmap(vaddr, file_size);

	if (!info) {
		sys_kobj_put(mem);
		return NULL;
	}

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

static errno_t elf_read_modules_2(const char *exec_name, stack_t *init_list, stack_t *bfs_list)
{
	hash_table_t libs_ht;
	if (!ht_stref_create(&libs_ht))
		return ENOMEM;

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
			stack_push(init_list, info);
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

			//DTRACE("DT_NEEDED(\"%s\")\n", needed);
			stack_push(&stack, needed);
		}

		DTRACE("Done listing.\n");
	}

	DTRACE("Loaded %d modules.\n", modules);

	// Next thing we need is the symbol resolution order,
	// i.e. breadth-first order of the dependency tree.
	compute_resolution_order(&libs_ht, init_list->array[init_list->stack_len - 1], bfs_list);

	assert(init_list->stack_len == bfs_list->stack_len);

	// Now that we're done, free the helper structures.
	// The elf_info structures are now referenced through the two lists.
	stack_destroy(&stack, NULL);
	stack_destroy(&enter_stack, NULL);
	ht_stref_destroy(&libs_ht, NULL);
	return EOK;

fail:
	// Free everything, including elf_info structures in the hash table.
	stack_destroy(init_list, NULL);
	stack_destroy(bfs_list, NULL);
	stack_destroy(&stack, NULL);
	stack_destroy(&enter_stack, NULL);
	ht_stref_destroy(&libs_ht, elf_info_free);
	return rc;
}

static void compute_bias(uintptr_t *vaddr_limit, struct elf_info *infos[], size_t infos_len)
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

errno_t elf_load_file_name2(const char *path, const char *cwd, const char *const args[], task_handle_t *out_task,
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

	task_handle_t child = sys_task_create(path);
	if (!child) {
		rc = ENOMEM;
		goto exit;
	}

	elf_map_modules(child, (struct elf_info **) init_list.array, init_list.stack_len);

	elf_spawn_task(child, path, init_list.array, bfs_list.array, init_list.stack_len, args, cwd, fd_stdin, fd_stdout, fd_stderr, vaddr_limit);

	*out_task = child;

exit:
	stack_destroy(&bfs_list, NULL);
	stack_destroy(&init_list, elf_info_free);
	return rc;
}

/** @}
 */
