#ifndef _NAMELESS_HASH_H_
#define _NAMELESS_HASH_H_

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
