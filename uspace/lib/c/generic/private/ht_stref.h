#ifndef _LIBC_HT_STREF_H_
#define _LIBC_HT_STREF_H_

#include <stdbool.h>
#include <adt/hash_table.h>

bool ht_stref_create(hash_table_t *);
void ht_stref_destroy(hash_table_t *, void (*destroy_fn)(void *));
bool ht_stref_insert(hash_table_t *, const char *, void *);
void *ht_stref_set(hash_table_t *, const char *, void *);
void *ht_stref_get(hash_table_t *, const char *);
void *ht_stref_remove(hash_table_t *, const char *);

#endif
