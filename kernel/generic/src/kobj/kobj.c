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

#include <kobj.h>
#include <mm/slab.h>
#include <adt/hash.h>
#include <proc/task.h>

static slab_cache_t *kobj_table_entry_cache;
static slab_cache_t *kobj_proxy_cache;

typedef struct {
	ht_link_t link;
	kobj_handle_t handle;
	kobj_t *kobj;
} kobj_table_entry_t;

struct kobj_proxy_ref {
	kobj_t kobj;
	kobj_proxy_t *outer;

	IRQ_SPINLOCK_DECLARE(lock);
	kobj_t *wrapped;
};

struct kobj_proxy {
	kobj_t kobj;

	struct kobj_proxy_ref inner;
};

void kobj_init(void)
{
	kobj_table_entry_cache = slab_cache_create("kobj_table_entry_t",
	    sizeof(kobj_table_entry_t), 0, NULL, NULL, 0);
	kobj_proxy_cache = slab_cache_create("kobj_proxy_t",
	    sizeof(kobj_proxy_t), 0, NULL, NULL, 0);
}

kobj_t *kobj_ref(kobj_t *kobj)
{
	if (kobj)
		refcount_up(&kobj->refcount);
	return kobj;
}

kobj_t *kobj_try_ref(kobj_t *kobj)
{
	if (kobj && refcount_try_up(&kobj->refcount))
		return kobj;
	else
		return NULL;
}

void kobj_put(kobj_t *kobj)
{
	if (kobj && refcount_down(&kobj->refcount)) {
		kobj->type->destroy(kobj);
	}
}

static void _outer_destroy(void *arg)
{
	kobj_proxy_t *proxy = arg;
	kobj_put(&proxy->inner.kobj);
}

static void _inner_destroy(void *arg)
{
	struct kobj_proxy_ref *inner = arg;

	irq_spinlock_lock(&inner->lock, true);
	if (inner->wrapped) {
		kobj_put(inner->wrapped);
		inner->wrapped = NULL;
	}
	irq_spinlock_unlock(&inner->lock, true);

	slab_free(kobj_proxy_cache, inner->outer);
}

static kobj_class_t kobj_class_proxy_outer = {
	.destroy = _outer_destroy,
};

static kobj_class_t kobj_class_proxy_inner = {
	.destroy = _inner_destroy,
};

/** Create a proxy object. The wrapped reference is consumed. */
kobj_proxy_t *kobj_proxy_create(kobj_t *wrapped)
{
	kobj_proxy_t *proxy = slab_alloc(kobj_proxy_cache, 0);
	if (!proxy)
		return NULL;

	kobj_initialize(&proxy->kobj, &kobj_class_proxy_outer);
	kobj_initialize(&proxy->inner.kobj, &kobj_class_proxy_inner);

	irq_spinlock_initialize(&proxy->inner.lock, "kobj_proxy_t::inner::lock");
	proxy->inner.outer = proxy;
	return proxy;
}

/** Get a proxy reference object for the reference wrapped by this proxy.
 * The returned reference acts for all intents and purposes like the original
 * reference passed kobj_proxy_create(), except that a call to
 * kobj_proxy_invalidate() may render it unusable without affecting the wrapped
 * object.
 */
kobj_t *kobj_proxy_get_inner(kobj_proxy_t *proxy)
{
	return kobj_ref(&proxy->inner.kobj);
}

/** Invalidate the proxy object.
 * The wrapped reference is destroyed and any future operations on
 * the associated proxy reference act as if called with invalid handle.
 * The associated proxy reference object still exists however, and must be
 * managed as usual with kobj_ref()/kobj_unref(). It is merely "empty".
 */
void kobj_proxy_invalidate(kobj_proxy_t *proxy)
{
	irq_spinlock_lock(&proxy->inner.lock, true);
	kobj_put(proxy->inner.wrapped);
	proxy->inner.wrapped = NULL;
	irq_spinlock_unlock(&proxy->inner.lock, true);
}

/**
 * Get object of the specified type, if possible.
 * Transparently unwraps inner proxy object.
 */
static void *kobj_get(kobj_t *kobj, const kobj_class_t *type)
{
	kobj_ref(kobj);

	while (kobj != NULL && kobj->type == &kobj_class_proxy_inner) {
		struct kobj_proxy_ref *inner = (void *) kobj;

		/*
		 * We need to ref it inside the lock, otherwise we might race ourselves
		 * into a use-after-free situation.
		 */
		irq_spinlock_lock(&inner->lock, true);
		kobj_t *wrapped = inner->wrapped ? kobj_ref(inner->wrapped) : NULL;
		irq_spinlock_unlock(&inner->lock, true);

		kobj_put(kobj);
		kobj = wrapped;
	}

	if (!kobj || kobj->type == type)
		return kobj;

	/* Incorrect type, act like we found nothing. */
	kobj_put(kobj);
	return NULL;
}

// Hash table operations.

static inline kobj_table_entry_t *get_table_entry(const ht_link_t *item)
{
	return hash_table_get_inst(item, kobj_table_entry_t, link);
}

static size_t refs_hash(const ht_link_t *item)
{
	return hash_mix32(get_table_entry(item)->handle);
}

static size_t refs_key_hash(const void *key)
{
	const kobj_handle_t *handle = key;
	return hash_mix32(*handle);
}

static bool refs_key_equal(const void *key, const ht_link_t *item)
{
	const kobj_handle_t *handle = key;
	return *handle == get_table_entry(item)->handle;
}

errno_t kobj_table_initialize(kobj_table_t *table)
{
	static const hash_table_ops_t refs_ops = {
		.hash = refs_hash,
		.key_hash = refs_key_hash,
		.key_equal = refs_key_equal,
	};

	table->handles = ra_arena_create();
	if (!table->handles) {
		return ENOMEM;
	}

	if (!ra_span_add(table->handles, 1, INT_MAX - 1) || !hash_table_create(&table->refs, 0, 0, &refs_ops)) {
		ra_arena_destroy(table->handles);
		return ENOMEM;
	}

	mutex_initialize(&table->lock, MUTEX_PASSIVE);

	return EOK;
}

void kobj_table_destroy(kobj_table_t *table)
{
	ra_arena_destroy(table->handles);
	hash_table_destroy(&table->refs);
}

void *kobj_table_lookup(kobj_table_t *table, kobj_handle_t handle, const kobj_class_t *type)
{
	if (!handle)
		return NULL;

	kobj_t *kobj = NULL;

	mutex_lock(&table->lock);

	ht_link_t *link = hash_table_find(&table->refs, &handle);
	if (link) {
		kobj = kobj_get(get_table_entry(link)->kobj, type);
	}

	mutex_unlock(&table->lock);

	return kobj;
}

void *kobj_table_shallow_lookup(kobj_table_t *table, kobj_handle_t handle)
{
	kobj_t *kobj = NULL;

	mutex_lock(&table->lock);

	ht_link_t *link = hash_table_find(&table->refs, &handle);
	if (link) {
		kobj = kobj_ref(get_table_entry(link)->kobj);
	}

	mutex_unlock(&table->lock);

	return kobj;
}

kobj_handle_t kobj_table_insert(kobj_table_t *table, void *kobj)
{
	kobj_table_entry_t *entry = slab_alloc(kobj_table_entry_cache, 0);
	if (!entry) {
		return 0;
	}

	uintptr_t hbase;
	if (!ra_alloc(table->handles, 1, 1, &hbase)) {
		slab_free(kobj_table_entry_cache, entry);
		return 0;
	}

	assert(hbase > 0);
	assert(hbase <= INT_MAX);

	kobj_handle_t handle = (kobj_handle_t) hbase;

	entry->handle = handle;
	entry->kobj = kobj;

	mutex_lock(&table->lock);
	hash_table_insert(&table->refs, &entry->link);
	mutex_unlock(&table->lock);

	// No touching entry beyond this point, it's not ours anymore.

	return handle;
}

kobj_t *kobj_table_remove(kobj_table_t *table, kobj_handle_t handle)
{
	if (!handle)
		return NULL;

	mutex_lock(&table->lock);
	ht_link_t *link = hash_table_find(&table->refs, &handle);
	if (link) {
		hash_table_remove_item(&table->refs, link);
	}
	mutex_unlock(&table->lock);

	if (!link) {
		return NULL;
	}

	kobj_table_entry_t *entry = get_table_entry(link);

	assert(entry->handle == handle);

	kobj_t *kobj = entry->kobj;

	ra_free(table->handles, (uintptr_t) handle, 1);
	slab_free(kobj_table_entry_cache, entry);

	return kobj;
}

sys_errno_t sys_kobj_put(sysarg_t handle)
{
	kobj_t *kobj = kobj_table_remove(&TASK->kobj_table, handle);
	if (!kobj)
		return ENOENT;

	kobj_put(kobj);
	return EOK;
}

/** @}
 */
