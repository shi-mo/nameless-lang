#include <errno.h>
#include <string.h>
#include "nameless.h"
#include "nameless/node.h"
#include "nameless/mm.h"

#define NLS_MSG_BROKEN_LIST "Broken list"

static nls_node* nls_node_new(nls_node_type_t type);
static void nls_list_item_free(nls_node *list);
static int nls_list_count(nls_node *list);
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
		nls_string_release(tree->nn_func.nf_name);
		break;
	case NLS_TYPE_ABSTRACTION:
		{
			nls_abstraction *abst = &(tree->nn_abst);

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

			nls_node_release(app->nap_args);
			nls_node_release(app->nap_func);
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
	node->nn_var.nv_next_ref = NULL;
	node->nn_var.nv_name = nls_string_grab(name);
	return node;
}

nls_node*
nls_function_new(nls_fp fp, char *name)
{
	nls_node *node;
	nls_string *str = nls_string_new(name);

	if (!name) {
		return NULL;
	}
	node = nls_node_new(NLS_TYPE_FUNCTION);
	if (!node) {
		nls_string_free(str);
		return NULL;
	}
	node->nn_func.nf_name = nls_string_grab(str);
	node->nn_func.nf_fp = fp;
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
	app->nap_func = nls_node_grab(func);
	app->nap_args  = nls_node_grab(arg);
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
		NLS_WARN(NLS_MSG_NOT_IMPLEMENTED);
		return EPERM;
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
