#include "nameless.h"
#include "nameless/node.h"
#include "nameless/mm.h"

#define NLS_MSG_BROKEN_LIST "Broken list"

nls_node*
nls_int_new(int val)
{
	nls_node *node = nls_new(nls_node);

	if (!node) {
		return NULL;
	}
	node->nn_type = NLS_TYPE_INT;
	node->nn_int = val;
	return node;
}

nls_node*
nls_var_new(nls_string *name)
{
	nls_node *node = nls_new(nls_node);

	if (!node) {
		return NULL;
	}
	node->nn_type = NLS_TYPE_VAR;
	node->nn_var.nv_name = nls_grab(name);
	return node;
}

nls_node*
nls_function_new(nls_function func)
{
	nls_node *node = nls_new(nls_node);

	if (!node) {
		return NULL;
	}
	node->nn_type = NLS_TYPE_FUNCTION;
	node->nn_func = func;
	return node;
}

nls_node*
nls_application_new(nls_node *func, nls_node *arg)
{
	nls_application *app;
	nls_node *node = nls_new(nls_node);

	if (!node) {
		return NULL;
	}
	node->nn_type = NLS_TYPE_APPLICATION;
	app = &(node->nn_app);
	app->na_func = nls_grab(func);
	app->na_arg  = nls_grab(arg);
	return node;
}

nls_node*
nls_list_new(nls_node *item)
{
	nls_list *list;
	nls_node *node = nls_new(nls_node);

	if (!node) {
		return NULL;
	}
	list = &(node->nn_list);
	node->nn_type = NLS_TYPE_LIST;
	list->nl_head = nls_grab(item);
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
	list->nl_rest = nls_grab(new);
	return 0;
}
