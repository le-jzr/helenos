#include "../private/ht_stref.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "../private/cc.h"

struct bucket {
	ht_link_t link;
	size_t hash;
	const char *key;
	void *value;
};

static size_t hash(const ht_link_t *link)
{
	const struct bucket *bucket = hash_table_get_inst(link, struct bucket, link);
	return bucket->hash;
}

static size_t key_hash(const void *key)
{
	size_t h = 0;

	for (const char *s = key; *s != '\0'; s++)
		h = 31 * h + *s;

	return h;
}

static bool equal(const ht_link_t *item1, const ht_link_t *item2)
{
	const struct bucket *bucket1 = hash_table_get_inst(item1, struct bucket, link);
	const struct bucket *bucket2 = hash_table_get_inst(item2, struct bucket, link);

	return bucket1->hash == bucket2->hash && strcmp(bucket1->key, bucket2->key) == 0;
}

static bool key_equal(const void *key, const ht_link_t *item)
{
	const struct bucket *bucket = hash_table_get_inst(item, struct bucket, link);
	return strcmp(key, bucket->key) == 0;
}

static void remove_callback(ht_link_t *item)
{
	struct bucket *bucket = hash_table_get_inst(item, struct bucket, link);
	free(bucket);
}

static hash_table_ops_t ht_ops = {
	.hash = hash,
	.key_hash = key_hash,
	.equal = equal,
	.key_equal = key_equal,
	.remove_callback = remove_callback,
};

INTERNAL bool ht_stref_create(hash_table_t *ht)
{
	return hash_table_create(ht, 0, 0, &ht_ops);
}

static bool apply_fn(ht_link_t *link, void *arg)
{
	void (*destroy_fn)(void *) = arg;

	struct bucket *bucket = hash_table_get_inst(link, struct bucket, link);
	destroy_fn(bucket->value);
	bucket->value = NULL;
	return true;
}

INTERNAL void ht_stref_destroy(hash_table_t *ht, void (*destroy_fn)(void *))
{
	if (destroy_fn)
		hash_table_apply(ht, apply_fn, destroy_fn);

	hash_table_destroy(ht);
}

INTERNAL bool ht_stref_insert(hash_table_t *ht, const char *key, void *value)
{
	struct bucket *bucket;

	ht_link_t *link = hash_table_find(ht, key);
	if (link) {
		return false;
	} else {
		bucket = malloc(sizeof(struct bucket));
		assert(bucket);
		bucket->hash = key_hash(key);
		bucket->key = key;
		bucket->value = value;
		bool success = hash_table_insert_unique(ht, &bucket->link);
		assert(success);
		return true;
	}
}

INTERNAL void *ht_stref_set(hash_table_t *ht, const char *key, void *value)
{
	assert(ht != NULL);
	assert(key != NULL);

	struct bucket *bucket;

	ht_link_t *link = hash_table_find(ht, key);
	if (link) {
		bucket = hash_table_get_inst(link, struct bucket, link);
		void *old = bucket->value;
		bucket->value = value;
		return old;
	} else {
		bucket = malloc(sizeof(struct bucket));
		assert(bucket);
		bucket->hash = key_hash(key);
		bucket->key = key;
		bucket->value = value;
		bool success = hash_table_insert_unique(ht, &bucket->link);
		assert(success);
		return NULL;
	}
}

INTERNAL void *ht_stref_get(hash_table_t *ht, const char *key)
{
	const ht_link_t *link = hash_table_find(ht, key);
	if (!link)
		return NULL;

	return hash_table_get_inst(link, struct bucket, link)->value;
}

INTERNAL void *ht_stref_remove(hash_table_t *ht, const char *key)
{
	ht_link_t *link = hash_table_find(ht, key);
	if (!link)
		return NULL;

	const struct bucket *bucket = hash_table_get_inst(link, struct bucket, link);
	void *val = bucket->value;
	hash_table_remove_item(ht, link);
	return val;
}
