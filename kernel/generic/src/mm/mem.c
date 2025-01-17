/*
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

/** @addtogroup kernel_generic
 * @{
 */

#include <mm/mem.h>

#include <abi/mm/as.h>
#include <align.h>
#include <arch/mm/page.h>
#include <mem.h>
#include <mm/km.h>
#include <mm/reserve.h>
#include <mm/slab.h>
#include <proc/task.h>
#include <stdlib.h>
#include <str_error.h>
#include <synch/spinlock.h>
#include <syscall/copy.h>

struct mem {
	/* Must keep this the first entry. */
	kobject_t kobject;

	/* Size in bytes. Must be a multiple of page size. */
	size_t size;

	mutex_t mutex;
	physaddr_t *pages;

	/* Flags allowed for mapping this mem. */
	int flags;
};

static slab_cache_t *mem_cache;

void mem_init(void)
{
	mem_cache = slab_cache_create("mem_t", sizeof(mem_t), 0, NULL, NULL, 0);
}

static void mem_destroy(kobject_t *arg)
{
	mem_t *mem = (mem_t *) arg;

	if (mem->flags & AS_AREA_LATE_RESERVE) {
		for (size_t i = 0; i < mem->size / PAGE_SIZE; i++) {
			if (mem->pages[i])
				frame_free(mem->pages[i], 1);
		}
	} else {
		for (size_t i = 0; i < mem->size / PAGE_SIZE; i++) {
			if (mem->pages[i])
				frame_free_noreserve(mem->pages[i], 1);
		}
		reserve_free(mem->size / PAGE_SIZE);
	}

	free(mem->pages);
	slab_free(mem_cache, mem);
}

const kobject_ops_t mem_kobject_ops = {
	.destroy = mem_destroy,
};

physaddr_t mem_lookup(mem_t *mem, size_t offset, bool alloc)
{
	if (mem == NULL)
		return 0;

	assert(offset % PAGE_SIZE == 0);
	assert(offset < mem->size);

	size_t page_num = offset / PAGE_SIZE;

	mutex_lock(&mem->mutex);

	physaddr_t addr = mem->pages[page_num];

	if (addr || !alloc) {
		mutex_unlock(&mem->mutex);
		return addr;
	}

	/* Allocate a new clean frame. */

	if (mem->flags & AS_AREA_LATE_RESERVE) {
		if (!reserve_try_alloc(1)) {
			mutex_unlock(&mem->mutex);
			return 0;
		}
	}

	uintptr_t page = km_temporary_page_get(&addr, FRAME_NO_RESERVE);

	assert(page);
	assert(addr);

	memset((void *) page, 0, PAGE_SIZE);
	km_temporary_page_put(page);

	mem->pages[page_num] = addr;
	mutex_unlock(&mem->mutex);

	return addr;
}

static bool mem_flags_valid(int flags)
{
	return !(flags & ~(AS_AREA_READ | AS_AREA_WRITE | AS_AREA_EXEC | AS_AREA_CACHEABLE | AS_AREA_LATE_RESERVE));
}

static mem_t *mem_create(size_t size, int flags, uspace_addr_t template)
{
	assert(size % PAGE_SIZE == 0);
	assert(mem_flags_valid(flags));

	if (template)
		flags &= ~AS_AREA_LATE_RESERVE;

	mem_t *mem = slab_alloc(mem_cache, FRAME_ATOMIC);
	if (!mem)
		return NULL;

	*mem = (mem_t) {
		.size = size,
		.flags = flags | AS_AREA_CACHEABLE,
		.pages = calloc(size / PAGE_SIZE, sizeof(physaddr_t)),
	};

	kobject_initialize(&mem->kobject, KOBJECT_TYPE_MEM);

	if (!mem->pages) {
		slab_free(mem_cache, mem);
		return NULL;
	}

	mutex_initialize(&mem->mutex, MUTEX_PASSIVE);

	size_t pages = size / PAGE_SIZE;

	if (!(flags & AS_AREA_LATE_RESERVE)) {
		if (!reserve_try_alloc(pages)) {
			mem_put(mem);
			return NULL;
		}
	}

	if (template) {
		physaddr_t frame;

		for (size_t i = 0; i < pages; i++) {
			uintptr_t page = km_temporary_page_get(&frame, FRAME_NO_RESERVE);
			assert(page);

			mem->pages[i] = frame;

			errno_t rc = copy_from_uspace((void *) page, template, PAGE_SIZE);
			km_temporary_page_put(page);
			if (rc != EOK) {
				mem_put(mem);
				return NULL;
			}

			template += PAGE_SIZE;
		}
	}

	return mem;
}

void mem_put(mem_t *mem)
{
	if (mem)
		kobject_put(&mem->kobject);
}

static errno_t mem_map(task_t *task, mem_t *mem, uintptr_t offset, size_t size, uintptr_t *vaddr, int flags)
{
	assert(task);

	bool cow = (flags & AS_AREA_COW) != 0;
	if (cow) {
		flags ^= AS_AREA_COW;
		flags |= AS_AREA_WRITE;
	}

	/* For now, only support COW mapping for read-only mem. */
	if (cow && (mem->flags & AS_AREA_WRITE))
		return EINVAL;

	mem_backend_t *backend = NULL;
	mem_backend_data_t backend_data = { };

	if (mem) {
		size_t allowed_size = mem->size;
		int allowed_flags = mem->flags | AS_AREA_CACHEABLE | AS_AREA_GUARD | AS_AREA_LATE_RESERVE;

		if (cow) {
			allowed_flags |= AS_AREA_WRITE;
		}

		if (flags & ~allowed_flags) {
			printf("refused flags, allowed: 0%o, proposed: 0%o \n", allowed_flags, flags);
			return EINVAL;
		}

		if (allowed_size < offset || allowed_size - offset < size) {
			printf("refused size\n");
			return EINVAL;
		}

		backend = &mem_backend;
		backend_data = (mem_backend_data_t) {
			.mem = mem,
			.mem_offset = offset,
			.mem_cow = cow,
		};
	} else {
		backend = &anon_backend;
	}

	// task_t.as field is immutable after creation and has its own internal synchronization,
	// so this should be safe even for a different task.
	as_area_t *area = as_area_create(task->as, flags, size,
	    AS_AREA_ATTR_NONE, backend, &backend_data, vaddr, 0);

	return area == NULL ? ENOMEM : EOK;
}

sys_errno_t sys_mem_map(cap_mem_handle_t mem_handle, sysarg_t offset, sysarg_t size, uspace_ptr_uintptr_t uspace_vaddr, sysarg_t flags)
{
	printf("map: mem_handle %zu, offset %" PRIx64 ", size %" PRIx64 ", flags %d\n",
	    (size_t)mem_handle, (uint64_t) offset, (uint64_t) size, (int)flags);

	mem_t *mem = NULL;
	if (mem_handle) {
		mem = (mem_t *) kobject_get(TASK, mem_handle, KOBJECT_TYPE_MEM);
		if (!mem)
			return ENOENT;
	}

	uintptr_t vaddr;
	errno_t rc = copy_from_uspace(&vaddr, uspace_vaddr, sizeof(vaddr));
	if (rc == EOK) {
		printf("vaddr %" PRIxPTR "\n", vaddr);

		rc = mem_map(TASK, mem, offset, size, &vaddr, flags);
		printf("error: %s\n", str_error(rc));
		if (rc == EOK) {
			copy_to_uspace(uspace_vaddr, &vaddr, sizeof(vaddr));
			// mem reference is held by the as_area_t.
			return EOK;
		}
	}

	mem_put(mem);
	return rc;
}

sysarg_t sys_mem_create(sysarg_t size, sysarg_t flags)
{
	mem_t *mem = mem_create(size, flags, 0);
	if (!mem)
		return 0;


	cap_handle_t handle;
	errno_t rc = cap_alloc(TASK, &handle);
	if (rc != EOK) {
		mem_put(mem);
		return 0;
	}

	cap_publish(TASK, handle, &mem->kobject);
	return (sysarg_t) handle;
}

/** @}
 */
