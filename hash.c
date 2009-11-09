/*
 * Nameless - A lambda calculation language.
 * Copyright (C) 2009 Yoshifumi Shimono <yoshifumi.shimono@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <errno.h>
#include "nameless.h"
#include "nameless/mm.h"
#include "nameless/hash.h"

#define NLS_MSG_HASH_ENOENT "No such hash entry"

static nls_hash_entry* nls_hash_entry_new(nls_string *key, nls_node *node);
static void nls_hash_entry_free(void *ptr);

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
			nls_release(table[i].nhe_next);
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
		ent->nhe_next = nls_grab(head->nhe_next);
		nls_release(head->nhe_next);
	}
	head->nhe_next = nls_grab(ent);
	hash->nh_num++;
	return 0;
}

#ifdef NLS_UNIT_TEST
static void
test_nls_hash_add(void)
{
	nls_hash hash;
	nls_string *key1 = nls_grab(nls_string_new("key1"));
	nls_string *key2 = nls_grab(nls_string_new("KEY2"));
	nls_node *item1 = nls_grab(nls_int_new(1));
	nls_node *item2 = nls_grab(nls_int_new(2));

	nls_hash_init(&hash);
	NLS_ASSERT_EQUALS(0, hash.nh_num);
	nls_hash_add(&hash, key1, item1);
	NLS_ASSERT_EQUALS(1, hash.nh_num);
	nls_hash_add(&hash, key2, item2);
	NLS_ASSERT_EQUALS(2, hash.nh_num);

	nls_release(key1);
	nls_release(key2);
	nls_release(item1);
	nls_release(item2);
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
		prev->nhe_next = nls_grab(ent->nhe_next);
	}
	nls_release(ent);
	hash->nh_num--;
	return 0;
}

#ifdef NLS_UNIT_TEST
static void
test_nls_hash_remove(void)
{
	nls_hash hash;
	nls_string *key1 = nls_grab(nls_string_new("WWW"));
	nls_string *key2 = nls_grab(nls_string_new("912"));
	nls_node *item1 = nls_grab(nls_int_new(1));
	nls_node *item2 = nls_grab(nls_int_new(2));

	nls_hash_init(&hash);
	nls_hash_add(&hash, key1, item1);
	nls_hash_add(&hash, key2, item2);

	NLS_ASSERT_EQUALS(2, hash.nh_num);
	nls_hash_remove(&hash, key1);
	NLS_ASSERT_EQUALS(1, hash.nh_num);
	nls_hash_remove(&hash, key2);
	NLS_ASSERT_EQUALS(0, hash.nh_num);

	nls_release(key1);
	nls_release(key2);
	nls_release(item1);
	nls_release(item2);
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
	new->nhe_key  = nls_grab(key);
	new->nhe_node = nls_grab(node);
	return new;
}

static void
nls_hash_entry_free(void *ptr)
{
	nls_hash_entry *ent = (nls_hash_entry*)ptr;

	if (ent->nhe_next) {
		nls_release(ent->nhe_next);
	}
	nls_release(ent->nhe_key);
	nls_release(ent->nhe_node);
	nls_free(ent);
}
