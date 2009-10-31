#include <errno.h>
#include <string.h>
#include "nameless.h"
#include "nameless/node.h"
#include "nameless/mm.h"

#define NLS_MSG_BROKEN_LIST "Broken list"

static nls_node* nls_node_new(nls_node_type type);
static void nls_list_item_free(nls_node *list);
static void nls_arg_ref_free(nls_arg_ref *ref);
static int nls_list_count(nls_node *list);
static int nls_strcmp(nls_string *s1, nls_string *s2);
static int nls_register_refs(nls_node **tree, nls_node *var, nls_arg_ref **ref);

nls_node*
nls_node_grab(nls_node *node)
{
	node->nn_ref++;
	return nls_grab(node);
}

void
nls_tree_free(nls_node *tree)
{
	if (--(tree->nn_ref) > 0) {
		nls_release(tree);
		return;
	}
	switch (tree->nn_type) {
	case NLS_TYPE_INT:
		break;
	case NLS_TYPE_VAR:
		nls_string_free(tree->nn_var.nv_name);
		break;
	case NLS_TYPE_FUNCTION:
		break;
	case NLS_TYPE_ABSTRACTION:
		{
			int i;
			nls_abstraction *abst = &(tree->nn_abst);

			for (i = 0; i < abst->nab_num_arg; i++) {
				nls_arg_ref_free(abst->nab_arg_ref[i]);
			}
			nls_release(abst->nab_arg_ref);
			nls_tree_free(abst->nab_def);
		}
		break;
	case NLS_TYPE_LIST:
		nls_list_item_free(tree);
		break;
	case NLS_TYPE_APPLICATION:
		{
			nls_application *app = &tree->nn_app;

			nls_tree_free(app->na_arg);
			nls_tree_free(app->na_func);
		}
		break;
	default:
		NLS_BUG(NLS_MSG_INVALID_NODE_TYPE);
		return;
	}
	nls_release(tree);
}

nls_node*
nls_int_new(int val)
{
	nls_node *node = nls_node_new(NLS_TYPE_INT);

	if (!node) {
		return NULL;
	}
	node->nn_int = val;
	return node;
}

nls_node*
nls_var_new(nls_string *name)
{
	nls_node *node = nls_node_new(NLS_TYPE_VAR);

	if (!node) {
		return NULL;
	}
	node->nn_var.nv_name = nls_grab(name);
	return node;
}

nls_node*
nls_function_new(nls_function func)
{
	nls_node *node = nls_node_new(NLS_TYPE_FUNCTION);

	if (!node) {
		return NULL;
	}
	node->nn_func = func;
	return node;
}

nls_node*
nls_abstraction_new(nls_node *vars, nls_node **def)
{
	int ret;
	int i, n = nls_list_count(vars);
	nls_abstraction *abst;
	nls_arg_ref **ref_ary;
	nls_node *node;
	nls_node **var, *tmp;

	if (!(ref_ary = nls_array_new(nls_arg_ref*, n))) {
		return NULL;
	}
	if (!(node = nls_new(nls_node))) {
		goto free_exit;
	}

	memset(ref_ary, 0, sizeof(nls_arg_ref*) * n);
	i = 0;
	nls_list_foreach(vars, &var, &tmp) {
		if ((ret = nls_register_refs(def, (*var), &ref_ary[i]))) {
			goto free_exit;
		}
	}
	node->nn_type = NLS_TYPE_ABSTRACTION;
	abst = &(node->nn_abst);
	abst->nab_num_arg = n;
	abst->nab_arg_ref = nls_grab(ref_ary);
	abst->nab_def = nls_node_grab(*def);
	return node;
free_exit:
	for (i = 0; i < n; i++) {
		if (ref_ary[i]) {
			nls_free(ref_ary[i]);
		}
	}
	if (ref_ary) {
		nls_free(ref_ary);
	}
	if (node) {
		nls_free(node);
	}
	return NULL;
}

nls_node*
nls_application_new(nls_node *func, nls_node *arg)
{
	nls_application *app;
	nls_node *node = nls_node_new(NLS_TYPE_APPLICATION);

	if (!node) {
		return NULL;
	}
	app = &(node->nn_app);
	app->na_func = nls_node_grab(func);
	app->na_arg  = nls_node_grab(arg);
	return node;
}

nls_node*
nls_list_new(nls_node *item)
{
	nls_list *list;
	nls_node *node = nls_node_new(NLS_TYPE_LIST);

	if (!node) {
		return NULL;
	}
	list = &(node->nn_list);
	list->nl_head = nls_node_grab(item);
	list->nl_rest = NULL;
	return node;
}

int
nls_list_add(nls_node *node, nls_node *item)
{
	nls_list *list;
	nls_node *new;

	list = &(node->nn_list);
	if (!list->nl_head) {
		NLS_BUG(NLS_MSG_BROKEN_LIST);
		return 1; /* dummy */
	}
	if (list->nl_rest) {
		return nls_list_add(list->nl_rest, item);
	}

	new = nls_list_new(item);
	if (!new) {
		return 1;
	}
	list->nl_rest = nls_node_grab(new);
	return 0;
}

static nls_node*
nls_node_new(nls_node_type type)
{
	nls_node *node = nls_new(nls_node);

	if (!node) {
		return NULL;
	}
	node->nn_type = type;
	node->nn_ref  = 0;
	return node;
}

static void
nls_list_item_free(nls_node *node)
{
	nls_list *list = &(node->nn_list);

	nls_tree_free(list->nl_head);
	if (list->nl_rest) {
		nls_tree_free(list->nl_rest);
	}
}

static void
nls_arg_ref_free(nls_arg_ref *ref)
{
	while (ref) {
		nls_arg_ref *next = ref->nar_next;

		nls_release(ref);
		ref = next;
	}
}

static int
nls_list_count(nls_node *list)
{
	nls_node **item, *tmp;
	int n = 0;

	nls_list_foreach(list, &item, &tmp) {
		n++;
	}
	return n;
}

static int
nls_strcmp(nls_string *s1, nls_string *s2)
{
	if (s1->ns_hash != s2->ns_hash) {
		return s1->ns_hash - s2->ns_hash;
	}
	if (s1->ns_len != s2->ns_len) {
		return s1->ns_len - s2->ns_len;
	}
	return strncmp(s1->ns_bufp, s2->ns_bufp, s1->ns_len);
}

static int
nls_register_refs(nls_node **tree, nls_node *var, nls_arg_ref **ref)
{
	switch ((*tree)->nn_type) {
	case NLS_TYPE_INT:
		return 0;
	case NLS_TYPE_VAR:
		if (!nls_strcmp((*tree)->nn_var.nv_name, var->nn_var.nv_name)) {
			nls_arg_ref *new_ref = nls_new(nls_arg_ref);

			if (!new_ref) {
				return ENOMEM;
			}
			new_ref->nar_next = *ref ? nls_grab(*ref) : NULL;
			new_ref->nar_node = tree;
			*ref = nls_grab(new_ref);
		}
		return 0;
	case NLS_TYPE_FUNCTION:
		return 0;
	case NLS_TYPE_ABSTRACTION:
		NLS_WARN(NLS_MSG_NOT_IMPLEMENTED);
		return ENOTSUP;
	case NLS_TYPE_APPLICATION:
		nls_register_refs(&((*tree)->nn_app.na_arg), var, ref);
		return 0;
	case NLS_TYPE_LIST:
		{
			nls_node **item, *tmp;

			nls_list_foreach((*tree), &item, &tmp) {
				nls_register_refs(item, var, ref);
			}
		}
		return 0;
	default:
		NLS_BUG(NLS_MSG_INVALID_NODE_TYPE);
		return EINVAL;
	}
}
