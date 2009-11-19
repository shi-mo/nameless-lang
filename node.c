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
#include <string.h>
#include "nameless.h"
#include "nameless/node.h"
#include "nameless/mm.h"

#define NLS_MSG_BROKEN_LIST "Broken list"

#define nls_INT_operations		nls_int_operations
#define nls_VAR_operations		nls_var_operations
#define nls_FUNCTION_operations		nls_function_operations
#define nls_ABSTRACTION_operations	nls_abstraction_operations
#define nls_APPLICATION_operations	nls_application_operations
#define nls_LIST_operations		nls_list_operations

#define nls_node_new(type) \
	_nls_node_new(NLS_TYPE_##type, &nls_##type##_operations)

static void nls_int_release(nls_node *tree);
static void nls_var_release(nls_node *tree);
static void nls_function_release(nls_node *tree);
static void nls_abstraction_release(nls_node *tree);
static void nls_application_release(nls_node *tree);
static void nls_list_release(nls_node *tree);

static nls_node* _nls_node_new(nls_node_type_t type, nls_node_operations *op);
static void nls_list_item_free(nls_node *node);
static nls_node* nls_list_tail_entry(nls_node *node);
static int nls_register_vars(nls_node **tree, nls_node *var);

static nls_node_operations nls_int_operations = {
	.nop_release = nls_int_release,
};

static nls_node_operations nls_var_operations = {
	.nop_release = nls_var_release,
};

static nls_node_operations nls_function_operations = {
	.nop_release = nls_function_release,
};

static nls_node_operations nls_abstraction_operations = {
	.nop_release = nls_abstraction_release,
};

static nls_node_operations nls_application_operations = {
	.nop_release = nls_application_release,
};

static nls_node_operations nls_list_operations = {
	.nop_release = nls_list_release,
};

void
nls_node_free(void *ptr)
{
	nls_node *node = (nls_node*)ptr;

	node->nn_op->nop_release(node);
	nls_free(node);
}

static void
nls_int_release(nls_node *tree)
{
	/* Nothing to do. */
}

static void
nls_var_release(nls_node *tree)
{
	nls_release(tree->nn_var.nv_name);
}

static void
nls_function_release(nls_node *tree)
{
	nls_release(tree->nn_func.nf_name);
}

static void
nls_abstraction_release(nls_node *tree)
{
	nls_abstraction *abst = &(tree->nn_abst);

	nls_release(abst->nab_vars);
	nls_release(abst->nab_def);
}

static void
nls_application_release(nls_node *tree)
{
	nls_application *app = &tree->nn_app;

	nls_release(app->nap_args);
	nls_release(app->nap_func);
}

static void
nls_list_release(nls_node *tree)
{
	nls_list_item_free(tree);
}

nls_node*
nls_int_new(int val)
{
	nls_node *node = nls_node_new(INT);

	if (!node) {
		return NULL;
	}
	node->nn_int = val;
	return node;
}

nls_node*
nls_var_new(nls_string *name)
{
	nls_node *node = nls_node_new(VAR);

	if (!node) {
		return NULL;
	}
	node->nn_var.nv_next_ref = NULL;
	node->nn_var.nv_name = nls_grab(name);
	return node;
}

nls_node*
nls_function_new(nls_fp fp, int num_args, char *name)
{
	nls_node *node;
	nls_string *str = nls_string_new(name);

	if (!name) {
		return NULL;
	}
	node = nls_node_new(FUNCTION);
	if (!node) {
		nls_string_free(str);
		return NULL;
	}
	node->nn_func.nf_num_args = num_args;
	node->nn_func.nf_name = nls_grab(str);
	node->nn_func.nf_fp = fp;
	return node;
}

nls_node*
nls_abstraction_new(nls_node *vars, nls_node *def)
{
	int ret;
	int i, n = nls_list_count(vars);
	nls_abstraction *abst;
	nls_node *node;
	nls_node **var, *tmp;

	if (!(node = nls_node_new(ABSTRACTION))) {
		return NULL;
	}

	i = 0;
	nls_list_foreach(vars, &var, &tmp) {
		if ((ret = nls_register_vars(&def, *var))) {
			goto free_exit;
		}
	}
	abst = &(node->nn_abst);
	abst->nab_num_args = n;
	abst->nab_vars = nls_grab(vars);
	abst->nab_def  = nls_grab(def);
	return node;
free_exit:
	nls_release(node);
	return NULL;
}

nls_node*
nls_application_new(nls_node *func, nls_node *args)
{
	nls_application *app;
	nls_node *node = nls_node_new(APPLICATION);

	if (!node) {
		return NULL;
	}
	app = &(node->nn_app);
	app->nap_func = nls_grab(func);
	app->nap_args = nls_grab(args);
	return node;
}

nls_node*
nls_list_new(nls_node *item)
{
	nls_list *list;
	nls_node *node = nls_node_new(LIST);

	if (!node) {
		return NULL;
	}
	list = &(node->nn_list);
	list->nl_head = nls_grab(item);
	list->nl_rest = NULL;
	return node;
}

int
nls_list_add(nls_node *ent, nls_node *item)
{
	nls_list *list;
	nls_node *tail, *new;

	tail = nls_list_tail_entry(ent);
	list = &(tail->nn_list);
	new = nls_list_new(item);
	if (!new) {
		return 1;
	}
	list->nl_rest = nls_grab(new);
	return 0;
}

void
nls_list_remove(nls_node **ent)
{
	nls_list *list;
	nls_node *tmp = *ent;

	if (!NLS_ISLIST(*ent)) {
		NLS_BUG(NLS_MSG_INVALID_NODE_TYPE);
		return;
	}
	list = &(tmp->nn_list);
	*ent = list->nl_rest;
	tmp->nn_list.nl_rest = NULL;
	nls_release(tmp);
}

#ifdef NLS_UNIT_TEST
static void
test_nls_list_remove(void)
{
	nls_node *list;
	nls_node *node1 = nls_int_new(1);
	nls_node *node2 = nls_int_new(2);
	nls_node *node3 = nls_int_new(3);

	list = nls_grab(nls_list_new(node1));
	nls_list_add(list, node2);
	nls_list_add(list, node3);

	nls_list_remove(&list);
	NLS_ASSERT_EQUALS(2, nls_list_count(list));
	nls_release(list);
}
#endif /* NLS_UNIT_TEST */

int
nls_list_concat(nls_node *ent1, nls_node *ent2)
{
	nls_list *list;
	nls_node *tail;

	tail = nls_list_tail_entry(ent1);
	list = &(tail->nn_list);
	list->nl_rest = nls_grab(ent2);
	return 0;
}

int
nls_list_count(nls_node *ent)
{
	nls_node **item, *tmp;
	int n = 0;

	nls_list_foreach(ent, &item, &tmp) {
		n++;
	}
	return n;
}

#ifdef NLS_UNIT_TEST
static void
test_nls_list_count_when_size1(void)
{
	nls_node *node = nls_int_new(1);
	nls_node *list = nls_list_new(node);
	NLS_ASSERT_EQUALS(1, nls_list_count(list));

	nls_node_free(list);
}

static void
test_nls_list_count_when_size3(void)
{
	nls_node *node1 = nls_int_new(1);
	nls_node *node2 = nls_int_new(2);
	nls_node *node3 = nls_int_new(3);
	nls_node *list = nls_list_new(node1);

	nls_list_add(list, node2);
	NLS_ASSERT_EQUALS(2, nls_list_count(list));

	nls_list_add(list, node3);
	NLS_ASSERT_EQUALS(3, nls_list_count(list));

	nls_node_free(list);
}
#endif /* NLS_UNIT_TEST */

static nls_node*
_nls_node_new(nls_node_type_t type, nls_node_operations *op)
{
	nls_node *node = nls_new(nls_node);

	if (!node) {
		return NULL;
	}
	node->nn_type = type;
	node->nn_op = op;
	return node;
}

static void
nls_list_item_free(nls_node *node)
{
	nls_list *list = &(node->nn_list);

	nls_release(list->nl_head);
	if (list->nl_rest) {
		nls_release(list->nl_rest);
	}
}

static nls_node*
nls_list_tail_entry(nls_node *node)
{
	nls_list *list;

	list = &(node->nn_list);
	if (!list->nl_head) {
		NLS_BUG(NLS_MSG_BROKEN_LIST);
		return NULL;
	}
	if (list->nl_rest) {
		return nls_list_tail_entry(list->nl_rest);
	}
	return node;
}

static int
nls_register_vars(nls_node **tree, nls_node *var)
{
	switch ((*tree)->nn_type) {
	case NLS_TYPE_INT:
		return 0;
	case NLS_TYPE_VAR:
		if (!nls_strcmp((*tree)->nn_var.nv_name, var->nn_var.nv_name)) {
			nls_node **next_ref = var->nn_var.nv_next_ref;

			(*tree)->nn_var.nv_next_ref = next_ref;
			var->nn_var.nv_next_ref = tree;
		}
		return 0;
	case NLS_TYPE_FUNCTION:
		return 0;
	case NLS_TYPE_ABSTRACTION:
		nls_register_vars(&((*tree)->nn_abst.nab_vars), var);
		nls_register_vars(&((*tree)->nn_abst.nab_def),  var);
		return 0;
	case NLS_TYPE_APPLICATION:
		nls_register_vars(&((*tree)->nn_app.nap_args), var);
		return 0;
	case NLS_TYPE_LIST:
		{
			nls_node **item, *tmp;

			nls_list_foreach((*tree), &item, &tmp) {
				nls_register_vars(item, var);
			}
		}
		return 0;
	default:
		NLS_BUG(NLS_MSG_INVALID_NODE_TYPE);
		return EINVAL;
	}
}
