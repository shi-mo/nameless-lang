#include <errno.h>
#include <string.h>
#include "nameless.h"
#include "nameless/node.h"
#include "nameless/mm.h"

#define NLS_MSG_BROKEN_LIST "Broken list"

static nls_node* nls_node_new(nls_node_type_t type);
static void nls_list_item_free(nls_node *list);
static int nls_list_count(nls_node *list);
static int nls_strcmp(nls_string *s1, nls_string *s2);
static int nls_register_vars(nls_node **tree, nls_node *var);

nls_node*
nls_node_grab(nls_node *node)
{
	return nls_grab(node);
}

void
nls_node_release(nls_node *tree)
{
	if (!nls_is_last_ref(tree)) {
		nls_release(tree);
		return;
	}
	switch (tree->nn_type) {
	case NLS_TYPE_INT:
		break;
	case NLS_TYPE_VAR:
		nls_string_release(tree->nn_var.nv_name);
		break;
	case NLS_TYPE_FUNCTION:
		break;
	case NLS_TYPE_ABSTRACTION:
		{
			nls_abstraction *abst = &(tree->nn_abst);
			nls_node **var, *tmp;

			nls_list_foreach(abst->nab_vars, &var, &tmp) {
				nls_node *ref;
				for (ref = (*var)->nn_var.nv_next; ref;
					ref = ref->nn_var.nv_next) {
					nls_node_release(ref);
				}
			}
			nls_node_release(abst->nab_vars);
			nls_node_release(abst->nab_def);
		}
		break;
	case NLS_TYPE_LIST:
		nls_list_item_free(tree);
		break;
	case NLS_TYPE_APPLICATION:
		{
			nls_application *app = &tree->nn_app;

			nls_node_release(app->na_arg);
			nls_node_release(app->na_func);
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
	node->nn_var.nv_next = NULL;
	node->nn_var.nv_name = nls_string_grab(name);
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
	nls_node *node;
	nls_node **var, *tmp;

	if (!(node = nls_node_new(NLS_TYPE_ABSTRACTION))) {
		return NULL;
	}

	i = 0;
	nls_list_foreach(vars, &var, &tmp) {
		if ((ret = nls_register_vars(def, *var))) {
			goto free_exit;
		}
	}
	abst = &(node->nn_abst);
	abst->nab_num_arg = n;
	abst->nab_vars = nls_node_grab(vars);
	abst->nab_def  = nls_node_grab(*def);
	return node;
free_exit:
	nls_node_release(node);
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
nls_node_new(nls_node_type_t type)
{
	nls_node *node = nls_new(nls_node);

	if (!node) {
		return NULL;
	}
	node->nn_type = type;
	return node;
}

static void
nls_list_item_free(nls_node *node)
{
	nls_list *list = &(node->nn_list);

	nls_node_release(list->nl_head);
	if (list->nl_rest) {
		nls_node_release(list->nl_rest);
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
nls_register_vars(nls_node **tree, nls_node *var)
{
	switch ((*tree)->nn_type) {
	case NLS_TYPE_INT:
		return 0;
	case NLS_TYPE_VAR:
		if (!nls_strcmp((*tree)->nn_var.nv_name, var->nn_var.nv_name)) {
			nls_node *next = var->nn_var.nv_next;

			(*tree)->nn_var.nv_next = next ? nls_node_grab(next) : NULL;
			var->nn_var.nv_next = nls_node_grab(*tree);
		}
		return 0;
	case NLS_TYPE_FUNCTION:
		return 0;
	case NLS_TYPE_ABSTRACTION:
		NLS_WARN(NLS_MSG_NOT_IMPLEMENTED);
		return EPERM;
	case NLS_TYPE_APPLICATION:
		nls_register_vars(&((*tree)->nn_app.na_arg), var);
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
