#ifndef _NAMELESS_NODE_H_
#define _NAMELESS_NODE_H_

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

#include "nameless/string.h"

#define NLS_ISINT(node)  (NLS_TYPE_INT == (node)->nn_type)
#define NLS_ISVAR(node)  (NLS_TYPE_VAR == (node)->nn_type)
#define NLS_ISAPP(node)  (NLS_TYPE_APPLICATION == (node)->nn_type)
#define NLS_ISLIST(node) (NLS_TYPE_LIST == (node)->nn_type)
#define NLS_INT_VAL(node) ((node)->nn_int)

typedef enum {
	NLS_TYPE_INT = 1,
	NLS_TYPE_VAR,
	NLS_TYPE_FUNCTION,
	NLS_TYPE_ABSTRACTION,
	NLS_TYPE_APPLICATION,
	NLS_TYPE_LIST,
} nls_node_type_t;

struct _nls_node;
typedef struct _nls_var {
	struct _nls_node **nv_next_ref;
	nls_string *nv_name;
} nls_var;

typedef struct _nls_abstraction {
	int nab_num_args;
	struct _nls_node *nab_vars;
	struct _nls_node *nab_def;
} nls_abstraction;

typedef struct _nls_application {
	struct _nls_node *nap_func;
	struct _nls_node *nap_args;
} nls_application;

typedef struct _nls_list {
	struct _nls_node *nl_head;
	struct _nls_node *nl_rest;
} nls_list;

typedef int (*nls_fp)(struct _nls_node*, struct _nls_node**);

typedef struct _nls_function {
	int nf_num_args;
	nls_string *nf_name;
	nls_fp nf_fp;
} nls_function;

typedef void (*nls_nop_release)(struct _nls_node*);

typedef struct _nls_node_operations {
	nls_nop_release nop_release;
} nls_node_operations;

typedef struct _nls_node {
	nls_node_type_t nn_type;
	nls_node_operations *nn_op;
	union {
		int nnu_int;
		nls_var nnu_var;
		nls_list nnu_list;
		nls_function nnu_func;
		nls_abstraction nnu_abst;
		nls_application nnu_app;
	} nn_u;
} nls_node;

#define nn_int  nn_u.nnu_int
#define nn_var  nn_u.nnu_var
#define nn_list nn_u.nnu_list
#define nn_func nn_u.nnu_func
#define nn_abst nn_u.nnu_abst
#define nn_app  nn_u.nnu_app

/**
 * Traverse all items in nls_list.
 * @see nls_list_reduce()
 */
#define nls_list_foreach(list, item, tmp) \
	for ( \
		(*(item)) = &((list)->nn_list.nl_head), \
			(*(tmp)) = (list)->nn_list.nl_rest; \
		(*(item)); \
		(*(item)) = ((*(tmp)) ? &((*(tmp))->nn_list.nl_head) : NULL), \
			(*(tmp)) = ((*(tmp)) ? \
				(*(tmp))->nn_list.nl_rest : NULL) \
	) \

#define nls_double_list_foreach(list1, list2, item1, item2, tmp1, tmp2) \
	for ( \
		((*(item1)) = &((list1)->nn_list.nl_head), \
			(*(tmp1)) = (list1)->nn_list.nl_rest, \
			(*(item2)) = &((list2)->nn_list.nl_head), \
			(*(tmp2)) = (list2)->nn_list.nl_rest); \
		(*(item1) && *(item2)); \
		(*(item1) = (*(tmp1) ? &((*(tmp1))->nn_list.nl_head) : NULL), \
			(*(tmp1)) = ((*(tmp1)) ? \
				(*(tmp1))->nn_list.nl_rest : NULL), \
			(*(item2)) = ((*(tmp2)) ? \
				&((*(tmp2))->nn_list.nl_head) : NULL), \
			(*(tmp2)) = ((*(tmp2)) ? \
				(*(tmp2))->nn_list.nl_rest : NULL)) \
	) \

void nls_node_free(void *ptr);
nls_node* nls_int_new(int val);
nls_node* nls_var_new(nls_string *name);
nls_node* nls_function_new(nls_fp fp, int num_args, char *name);
nls_node* nls_abstraction_new(nls_node *vars, nls_node *def);
nls_node* nls_application_new(nls_node *func, nls_node *args);
nls_node* nls_list_new(nls_node *node);
int nls_list_add(nls_node *ent, nls_node *item);
void nls_list_remove(nls_node **ent);
int nls_list_concat(nls_node *ent1, nls_node *ent2);
int nls_list_count(nls_node *ent);

#endif /* _NAMELESS_NODE_H_ */
