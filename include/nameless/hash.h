#ifndef _NAMELESS_HASH_H_
#define _NAMELESS_HASH_H_

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

#include "nameless/node.h"

#define NLS_HASH_WIDTH 251

typedef struct _nls_hash_entry {
	struct _nls_hash_entry *nhe_next;
	nls_string *nhe_key;
	nls_node *nhe_node;
} nls_hash_entry;

typedef struct _nls_hash {
	int nh_num;
	nls_hash_entry nh_table[NLS_HASH_WIDTH];
} nls_hash;

void nls_hash_init(nls_hash *hash);
void nls_hash_term(nls_hash *hash);
int nls_hash_add(nls_hash *hash, nls_string *key, nls_node *item);
int nls_hash_remove(nls_hash *hash, nls_string *key);
nls_hash_entry* nls_hash_search(nls_hash *hash, nls_string *key, nls_hash_entry **prevp);

#endif /* _NAMELESS_HASH_H_ */
