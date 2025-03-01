/*
 * Copyright (c) 2025 Jiří Zárevúcky
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

#include <fibril_synch.h>
#include <mem.h>
#include <stdio.h>
#include <protocol/root.h>
#include <adt/array.h>
#include <adt/hash.h>
#include <adt/hash_table.h>
#include <string.h>
#include <ipc_b.h>

struct entry {
    ht_link_t link;
    char *name;
    ipc_object_t *obj;
    adt_array(ipc_object_t *) waiters;
};

static size_t _hash(const ht_link_t *item)
{
    auto entry = hash_table_get_inst(item, struct entry, link);
    return hash_string(entry->name);
}

static size_t _key_hash(const void *key)
{
    return hash_string(key);
}

static bool _equal(const ht_link_t *item1, const ht_link_t *item2)
{
    auto entry1 = hash_table_get_inst(item1, struct entry, link);
    auto entry2 = hash_table_get_inst(item2, struct entry, link);

    return strcmp(entry1->name, entry2->name) == 0;
}

static bool _key_equal(const void *key, size_t hash, const ht_link_t *item)
{
    auto entry = hash_table_get_inst(item, struct entry, link);

    return strcmp(entry->name, key) == 0;
}

static void _remove_callback(ht_link_t *link)
{
    auto entry = hash_table_get_inst(link, struct entry, link);

    adt_array_foreach(&entry->waiters, pwaiter) {
        ipc_object_put(*pwaiter);
    }

    adt_array_free(&entry->waiters);
    ipc_object_put(entry->obj);
    free(entry->name);
    free(entry);
}

static const hash_table_ops_t _hash_ops = {
    .hash = _hash,
    .key_hash = _key_hash,
    .equal = _equal,
    .key_equal = _key_equal,
    .remove_callback = _remove_callback,
};

static FIBRIL_MUTEX_INITIALIZE(_lock);
static hash_table_t _table;

static void _initialize_table()
{
    if (!hash_table_create(&_table, 0, 0, &_hash_ops))
        panic("out of memory");
}

static struct entry *_lookup_entry(const char *id)
{
    fibril_mutex_lock(&_lock);
    auto link = hash_table_find(&_table, id);
    return link ? hash_table_get_inst(link, struct entry, link) : NULL;
    fibril_mutex_unlock(&_lock);
}

static ipc_root_retval_t _register(const char *id, ipc_object_t *obj)
{
    auto entry = palloc(struct entry, 1);
    if (!entry)
        return ipc_root_failure;

    *entry = (struct entry) {
        .name = strdup(id),
    };

    if (!entry->name) {
        free(entry);
        return ipc_root_failure;
    }

    fibril_mutex_lock(&_lock);
    hash_table_remove(&_table, id);
    hash_table_insert(&_table, &entry->link);
    fibril_mutex_unlock(&_lock);
    return ipc_root_success;
}

static ipc_object_t *_get(const char *id)
{
    auto entry = _lookup_entry(id);
    return entry ? entry->obj : NULL;
}

int main(int argc, char **argv)
{
    printf("%s: HelenOS IPC Root Server\n", argv[0]);

    _initialize_table();

    const ipc_root_server_ops_t ops = {
        .obj_register = _register,
        .obj_get = _get,
    };

    ipc_root_serve(&ops);
}
