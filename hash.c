#include <errno.h>
#include "nameless.h"
#include "nameless/mm.h"
#include "nameless/hash.h"

#define NLS_MSG_HASH_ENOENT "No such hash entry"

static nls_hash_entry* nls_hash_entry_new(nls_string *key, nls_node *node);
static nls_hash_entry* nls_hash_entry_grab(nls_hash_entry *ent);
static void nls_hash_entry_release(nls_hash_entry *ent);

void
nls_hash_init(nls_hash *hash)
{
	int i;
	nls_hash_entry *table = hash->nh_table;

	for (i = 0; i < NLS_HASH_WIDTH; i++) {
		nls_hash_entry *ent = &table[i];

		ent->nhe_next = NULL;
		ent->nhe_key  = NULL;
		ent->nhe_node = NULL;
	}
	hash->nh_num = 0;
}

void
nls_hash_term(nls_hash *hash)
{
	int i;
	nls_hash_entry *table = hash->nh_table;

	for (i = 0; i < NLS_HASH_WIDTH; i++) {
		if (table[i].nhe_next) {
			nls_hash_entry_release(table[i].nhe_next);
		}
		table[i].nhe_next = NULL;
	}
	hash->nh_num = 0;
}

/**
 * Search for hash.
 * @param[in]  hash  Target hash pointer
 * @param[in]  key   Search key
 * @param[out] prevp Pointer of previous hash entry set when search success
 * @return !NULL Hash entry found
 * @return  NULL No such entry
 */
nls_hash_entry*
nls_hash_search(nls_hash *hash, nls_string *key, nls_hash_entry **prevp)
{
	nls_hash_entry *ent, *prev;
	nls_hash_entry *head = &hash->nh_table[key->ns_hash];

	for (ent = head->nhe_next, prev = head;
		ent; prev = ent, ent = ent->nhe_next) {
		if (!nls_strcmp(key, ent->nhe_key)) {
			*prevp = prev;
			return ent;
		}
	}
	*prevp = NULL;
	return NULL;
}

int
nls_hash_add(nls_hash *hash, nls_string *key, nls_node *item)
{
	nls_hash_entry *ent  = nls_hash_entry_new(key, item);
	nls_hash_entry *head = &hash->nh_table[key->ns_hash];

	if (!ent) {
		return ENOMEM;
	}
	if (head->nhe_next) {
		ent->nhe_next = nls_hash_entry_grab(head->nhe_next);
		nls_hash_entry_release(head->nhe_next);
	}
	head->nhe_next = nls_hash_entry_grab(ent);
	hash->nh_num++;
	return 0;
}

#ifdef NLS_UNIT_TEST
static void
test_nls_hash_add(void)
{
	nls_hash hash;
	nls_string *key1 = nls_string_grab(nls_string_new("key1"));
	nls_string *key2 = nls_string_grab(nls_string_new("KEY2"));
	nls_node *item1 = nls_node_grab(nls_int_new(1));
	nls_node *item2 = nls_node_grab(nls_int_new(2));

	nls_hash_init(&hash);
	NLS_ASSERT_EQUALS(0, hash.nh_num);
	nls_hash_add(&hash, key1, item1);
	NLS_ASSERT_EQUALS(1, hash.nh_num);
	nls_hash_add(&hash, key2, item2);
	NLS_ASSERT_EQUALS(2, hash.nh_num);

	nls_string_release(key1);
	nls_string_release(key2);
	nls_node_release(item1);
	nls_node_release(item2);
	nls_hash_term(&hash);
}
#endif /* NLS_UNIT_TEST */

int
nls_hash_remove(nls_hash *hash, nls_string *key)
{
	nls_hash_entry *prev;
	nls_hash_entry *ent = nls_hash_search(hash, key, &prev);

	if (!ent) {
		NLS_ERROR(NLS_MSG_HASH_ENOENT);
		return EINVAL;
	}
	if (ent->nhe_next) {
		prev->nhe_next = nls_hash_entry_grab(ent->nhe_next);
	}
	nls_hash_entry_release(ent);
	hash->nh_num--;
	return 0;
}

#ifdef NLS_UNIT_TEST
static void
test_nls_hash_remove(void)
{
	nls_hash hash;
	nls_string *key1 = nls_string_grab(nls_string_new("WWW"));
	nls_string *key2 = nls_string_grab(nls_string_new("912"));
	nls_node *item1 = nls_node_grab(nls_int_new(1));
	nls_node *item2 = nls_node_grab(nls_int_new(2));

	nls_hash_init(&hash);
	nls_hash_add(&hash, key1, item1);
	nls_hash_add(&hash, key2, item2);

	NLS_ASSERT_EQUALS(2, hash.nh_num);
	nls_hash_remove(&hash, key1);
	NLS_ASSERT_EQUALS(1, hash.nh_num);
	nls_hash_remove(&hash, key2);
	NLS_ASSERT_EQUALS(0, hash.nh_num);

	nls_string_release(key1);
	nls_string_release(key2);
	nls_node_release(item1);
	nls_node_release(item2);
	nls_hash_term(&hash);
}
#endif /* NLS_UNIT_TEST */

static nls_hash_entry*
nls_hash_entry_new(nls_string *key, nls_node *node)
{
	nls_hash_entry *new = nls_new(nls_hash_entry);

	if (!new) {
		return NULL;
	}
	new->nhe_next = NULL;
	new->nhe_key  = nls_string_grab(key);
	new->nhe_node = nls_node_grab(node);
	return new;
}

static nls_hash_entry*
nls_hash_entry_grab(nls_hash_entry *ent)
{
	return nls_grab(ent);
}

static void
nls_hash_entry_release(nls_hash_entry *ent)
{
	if (!nls_is_last_ref(ent)) {
		nls_release(ent);
		return;
	}
	if (ent->nhe_next) {
		nls_hash_entry_release(ent->nhe_next);
	}
	nls_string_release(ent->nhe_key);
	nls_node_release(ent->nhe_node);
	nls_release(ent);
}
