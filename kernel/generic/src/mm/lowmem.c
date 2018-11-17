
IRQ_SPINLOCK_STATIC_INITIALIZE_NAME(_lowmem_lock, "lowmem-lock");
static ODICT_INITIALIZE(_lowmem_odict, lowmem_odict_key, lowmem_odict_cmp);
static odlink_t *_lowmem_next = NULL;

static void *lowmem_alloc_from(size_t alignment, size_t size, odlink_t *link)
{
	memory_node_t *n = odict_get_instance(next, memory_node_t, odlink);

	physptr_t start = ALIGN_UP(n->start, alignment);
	physptr_t offset = (start - n->start);
	if (offset >= n->size)
		return NULL;

	physptr_t psize = n->size - offset;
	if (psize < size)
		return NULL;

	/* The region is large enough. */

	if (psize > size)

}

/**
 * Lowmem allocator.
 * Never needs to modify page tables because it's nice like that.
 *
 * Access to memory returned by lowmem allocator may never cause nested
 * processor exceptions (page fault or TLB miss). How this is achieved
 * is entirely up to the platform.
 * This allows it to be used for kernel page tables and exception stacks.
 *
 * @return  Virtual address of the returned memory.
 *          To get physical address, use KA2PA().
 */
void *lowmem_alloc(size_t alignment, size_t size)
{
	/* This is a variant of next-fit strategy. */

	assert(size % PAGE_SIZE == 0);
	assert(size > 0);
	assert(alignment >= PAGE_SIZE);
	assert(ispwr2(alignment));

	odlink_t *next = _lowmem_next;
	if (!next)
		next = odict_first(&_lowmem_odict);

	while (next != NULL) {


		next = odlink_next(next, &_lowmem_odict);
	}

	while


}

void lowmem_free(void *ptr, size_t alignment, size_t size)
{
}

void lowmem_provide(void *ptr, size_t size)
{
}

