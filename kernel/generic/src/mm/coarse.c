/*
 * Copyright (c) 2018 Jiří Zárevúcky
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

/** @file
 * Coarse memory allocator
 *
 * This file implements the coarse memory allocator, which operates at page
 * granularity. All allocations must be multiples of PAGE_SIZE and the returned
 * address is always aligned to PAGE_SIZE.
 *
 * The allocator implements both a physical memory allocator and kernel virtual
 * memory allocator. It is entirely self-contained and does not use any other
 * memory allocation structures present in the kernel. Only low-level page
 * mapping API is used.
 *
 * For simplicity, this file only manages free space, and as such it doesn't
 * provide support for allocated memory metadata. This has a number of
 * advantages, including the fact that the management structures don't tie up
 * any memory. Only memory overhead incurred by this allocator is due to
 * virtual memory fragmentation, which can be mitigated to some extent.
 *
 * For physical memory zone
 * management, including physical frame refcounts, see zone.c
 *
 * If you are looking for userspace memory allocator, see mm/as.c
 *
 * For allocator of small kernel objects, see mm/slab.c and mm/malloc.c
 *
 * This file replaces km.c and frame.c, which were removed.
 */

/* The allocator is currently fully serialized by a single lock. */
static IRQ_SPINLOCK_DECLARE(lock);

typedef union memory_node {
	struct {
		odlink_t odlink;

		/* These are used for either physical or virtual memory. */
		physptr_t start;
		physptr_t size;
	};
	union memory_node *next;
} memory_node_t;

/* odict node allocator
 *
 * This is a pocket version of small object allocator used solely for
 * coarse allocator's metadata.
 */

static memory_node_t *_freelist;

static memory_node_t *_freelist_refill(void *page)
{
	if (!page)
		return NULL;

	memory_node_t *n = page;
	const size_t num = PAGE_SIZE / sizeof(*n);
	for (int i = 0; i < num - 1; i++) {
		n[i].next = &n[i + 1];
	}

	n[num - 1].next = NULL;
	return n;
}

static memory_node_t *memory_node_alloc(void)
{
	if (!_freelist) {
		/*
		 * We only track unused pages, which means coarse_alloc
		 * will never need to allocate a memory node.
		 */
		_freelist = _freelist_refill(coarse_alloc(PAGE_SIZE));
	}

	if (!_freelist)
		return NULL;

	memory_node_t *n = _freelist;
	_freelist = n->next;
	return n;
}

static void memory_node_free(memory_node_t *n)
{
	// TODO: deallocate full pages when there are too many free nodes
	n->next = _freelist;
	_freelist = n;
}





