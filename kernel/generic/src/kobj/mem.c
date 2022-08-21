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

#include <align.h>
#include <abi/mm/as.h>
#include <arch/mm/page.h>
#include <kobj.h>
#include <synch/spinlock.h>
#include <mm/slab.h>
#include <mm/km.h>
#include <mem.h>
#include <syscall/copy.h>

enum {
	ENTRY_ADDR_MASK = UINT64_C(0x7ffffffffffff000),
	// Low 12 bits are a reference count.
	ENTRY_REFCNT_MASK = UINT64_C(0xfff),

	// Set the number entries to get page-sized tables.
	DIRECTORY_TABLE_BITS = PAGE_WIDTH - 3,
	DIRECTORY_TABLE_MASK = (1 << DIRECTORY_TABLE_BITS) - 1,
	DIRECTORY_LEN = 1 << DIRECTORY_TABLE_BITS,
};

#define ENTRY_ADDR(entry) ((entry) & ENTRY_ADDR_MASK)
#define ENTRY_REFCNT(entry) ((entry) & ENTRY_REFCNT_MASK)

struct directory_table {
	phys_addr_t entries[DIRECTORY_LEN];
};

_Static_assert(sizeof(struct directory_table) == PAGE_SIZE, "directory table size != PAGE_SIZE");

struct mem {
	// Must keep this the first entry.
	kobj_t kobj;

	irq_spinlock_t lock;

	// Size in bytes. Must be a multiple of page size.
	// This determines the number of levels in directory.
	uint64_t size;

	// Page size of this memory. Must be 0 or a power of 2.
	// May be greater than PAGE_SIZE, in which case we
	// will try to use the largest supported architectural
	// large pages less or equal to the page size specified.
	// (eventually, not currently implemented)
	size_t page_size;

	phys_addr_t root_entry;

	// Counts the number of virtual pages across all address spaces
	// mapping this mem with the given permission.
	// Relevant when someone is attempting to downgrade permissions,
	// which only works if no mappings with the removed permission exist.
	uint64_t readable_count;
	uint64_t writable_count;
	uint64_t executable_count;

	// Flags allowed for mapping this mem.
	int flags;

	// TODO: If true, only remaining references to this mem are
	// those in virtual address space mappings.
	// Notably, that means any frames whose reference count
	// drops to zero can be immediately deallocated.
	//
	// bool floating;
};

static slab_cache_t *mem_cache;

void mem_init(void)
{
	mem_cache = slab_cache_create("mem_t", sizeof(mem_t), 0, NULL, NULL, 0);
}

static int level_count(uint64_t size, size_t page_size)
{
	size /= page_size;

	int level = 0;

	while (size > 0) {
		level++;
		size >>= DIRECTORY_TABLE_BITS;
	}

	return level;
}

errno_t mem_change_flags(mem_t *mem, int flags)
{
	if (!mem_flags_valid(flags))
		return EINVAL;

	bool success = true;

	irq_spinlock_lock(&mem->lock, true);

	if (!(flags & AS_AREA_READ) && mem->readable_count > 0)
		success = false;

	if (!(flags & AS_AREA_WRITE) && mem->writable_count > 0)
		success = false;

	if (!(flags & AS_AREA_EXEC) && mem->executable_count > 0)
		success = false;

	if (success)
		mem->flags = flags | AS_AREA_CACHEABLE;

	irq_spinlock_unlock(&mem->lock, true);

	return success ? EOK : EINVAL;
}

static inline struct directory_table *tableof(phys_addr_t entry)
{
	return (struct directory_table *) PA2KA(ENTRY_ADDR(entry));
}

static void expand_subtree(mem_t *mem, phys_addr_t *entry)
{
	assert(irq_spinlock_locked(&mem->lock));

	// Unlock the spinlock to call frame_alloc().
	irq_spinlock_unlock(&mem->lock, true);
	phys_addr_t frame = frame_alloc(1, FRAME_LOWMEM, 0);
	assert((frame & ~ENTRY_ADDR_MASK) == 0);
	irq_spinlock_lock(&mem->lock, true);

	if (!frame)
		return;

	// We must hold a reference over this range to call mem_lookup(),
	// so it's guaranteed `entry` remains valid through the unlocked section.

	if (ENTRY_ADDR(*entry)) {
		// Someone else beat us. Dealloc our frame.
		irq_spinlock_unlock(&mem->lock, true);
		frame_free(frame, 1);
		irq_spinlock_lock(&mem->lock, true);

		assert(ENTRY_ADDR(*entry));
	} else {
		// Move the reference count into the newly allocated table.
		phys_addr_t refcnt = *entry;
		struct directory_table *dir = (void *) PA2KA(frame);
		for (int i = 0; i < DIRECTORY_LEN; i++) {
			dir->entries[i] = refcnt;
		}

		// Set the entry.
		*entry = frame;
	}
}

static void alloc_frame(mem_t *mem, phys_addr_t *entry)
{
	assert(irq_spinlock_locked(&mem->lock));

	uintptr_t frame;

	// Unlock the spinlock to call frame_alloc().
	irq_spinlock_unlock(&mem->lock, true);
	uintptr_t page = km_temporary_page_get(&frame, 0);
	if (page) {
		memset((void *) page, 0, PAGE_SIZE);
		km_temporary_page_put(page);
	}
	irq_spinlock_lock(&mem->lock, true);

	if (!frame)
		return;

	if (ENTRY_ADDR(*entry)) {
		// Someone else beat us. Dealloc our frame.
		irq_spinlock_unlock(&mem->lock, true);
		frame_free(frame, 1);
		irq_spinlock_lock(&mem->lock, true);

		assert(ENTRY_ADDR(*entry));
	} else {
		*entry |= frame;
	}
}

uintptr_t mem_read_word(mem_t *mem, uint64_t offset)
{
	assert(offset % sizeof(uintptr_t) == 0);

	uint64_t page_offset = ALIGN_DOWN(offset, PAGE_SIZE);

	phys_addr_t frame = mem_lookup(mem, page_offset, false);
	assert(frame);

	// TODO: proper mapping
	assert(frame < config.identity_size);

	void *src = (void *) PA2KA(frame) + (offset - page_offset);
	return *(uintptr_t *) src;
}

errno_t mem_write_from_uspace(mem_t *mem, uint64_t offset, uintptr_t src, size_t size)
{
	errno_t rc;

	assert(size <= mem->size);
	assert(offset <= mem->size - size);

	// Write page by page, allocating backing frames if needed.

	uint64_t page_offset = ALIGN_DOWN(offset, PAGE_SIZE);

	if (page_offset < offset) {
		// Write partial first page.

		size_t sz = page_offset + PAGE_SIZE - offset;
		if (sz > size)
			sz = size;

		phys_addr_t frame = mem_lookup(mem, page_offset, true);

		// TODO: proper mapping
		assert(frame < config.identity_size);

		rc = copy_from_uspace((void *) PA2KA(frame) + (offset - page_offset), src, sz);
		if (rc != EOK)
			return rc;

		src += sz;
		size -= sz;

		page_offset += PAGE_SIZE;
	}

	// Write all full pages.
	while (size >= PAGE_SIZE) {
		phys_addr_t frame = mem_lookup(mem, page_offset, true);

		// TODO: proper mapping
		assert(frame < config.identity_size);

		rc = copy_from_uspace((void *) PA2KA(frame), src, PAGE_SIZE);
		if (rc != EOK)
			return rc;

		src += PAGE_SIZE;
		size -= PAGE_SIZE;
		page_offset += PAGE_SIZE;
	}

	// Write partial last page.
	if (size > 0) {
		phys_addr_t frame = mem_lookup(mem, page_offset, true);

		// TODO: proper mapping
		assert(frame < config.identity_size);

		return copy_from_uspace((void *) PA2KA(frame), src, size);
	}

	return EOK;
}

static phys_addr_t mem_lookup_locked(mem_t *mem, uint64_t offset, bool alloc)
{
	int level = level_count(mem->size, PAGE_SIZE);
	offset /= mem->page_size;
	int index_shift = level * DIRECTORY_TABLE_BITS;

	phys_addr_t *entry = &mem->root_entry;

	for (; level > 0; level--) {
		if (alloc && !ENTRY_ADDR(*entry))
			expand_subtree(mem, entry);

		if (!ENTRY_ADDR(*entry))
			return 0;

		index_shift -= DIRECTORY_TABLE_BITS;
		int64_t index = (offset >> index_shift) & DIRECTORY_TABLE_MASK;
		entry = &tableof(*entry)->entries[index];
	}

	if (alloc && !ENTRY_ADDR(*entry))
		alloc_frame(mem, entry);

	return ENTRY_ADDR(*entry);
}

phys_addr_t mem_lookup(mem_t *mem, uint64_t offset, bool alloc)
{
	if (mem == NULL)
		return 0;

	assert(offset % PAGE_SIZE == 0);
	assert(offset < mem->size);

	irq_spinlock_lock(&mem->lock, true);
	phys_addr_t addr = mem_lookup_locked(mem, offset, alloc);
	irq_spinlock_unlock(&mem->lock, true);
	return addr;
}

#if 0

void mem_range_ref(uint64_t offset, size_t size, int flags)
{
	// TODO
}

void mem_range_unref(uint64_t offset, size_t size, int flags)
{
	// TODO

	// In the future, we will free unreferenced frames in live mem_t
	// if mem->floating is true. For now, mem_destroy clears everything.
}

#endif

static void free_subtree(phys_addr_t entry, int level)
{
	if (ENTRY_ADDR(entry) == 0)
		return;

	if (level > 0) {
		struct directory_table *dir = tableof(entry);

		for (int i = 0; i < DIRECTORY_LEN; i++)
			free_subtree(dir->entries[i], level - 1);
	}

	frame_free(ENTRY_ADDR(entry), 1);
}

bool mem_flags_valid(int flags)
{
	return !(flags & ~(AS_AREA_READ|AS_AREA_WRITE|AS_AREA_EXEC|AS_AREA_CACHEABLE));
}

static bool is_pow2(uint64_t n)
{
	return (n & (n - 1)) == 0;
}

mem_t *mem_create(uint64_t size, size_t page_size, int flags)
{
	if (!mem_flags_valid(flags))
		return NULL;

	if (page_size < PAGE_SIZE)
		page_size = PAGE_SIZE;

	if (!is_pow2(page_size))
		return NULL;

	if (!IS_ALIGNED(size, page_size))
		return NULL;

	mem_t *mem = slab_alloc(mem_cache, 0);
	if (!mem)
		return NULL;

	*mem = (mem_t) {0};

	kobj_initialize(&mem->kobj, KOBJ_CLASS_MEM);
	irq_spinlock_initialize(&mem->lock, "mem_t.lock");
	mem->size = size;
	mem->page_size = page_size;
	mem->flags = flags | AS_AREA_CACHEABLE;
	return mem;
}

uint64_t mem_size(mem_t *mem)
{
	return mem->size;
}

int mem_flags(mem_t *mem)
{
	// TODO
	irq_spinlock_lock(&mem->lock, true);
	int flags = mem->flags;
	irq_spinlock_unlock(&mem->lock, true);
	return flags;
}

void mem_put(mem_t *mem)
{
	if (mem)
		kobj_put(&mem->kobj);
}

static void mem_destroy(void *arg)
{
	mem_t *mem = arg;
	free_subtree(mem->root_entry, level_count(mem->size, mem->page_size));
	slab_free(mem_cache, mem);
}

const kobj_class_t kobj_class_mem = {
	.destroy = mem_destroy,
};

/** @}
 */
