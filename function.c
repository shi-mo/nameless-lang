#include "nameless/node.h"
#include "nameless/mm.h"

int
nls_func_add(nls_node *arg, nls_node **out)
{
	nls_node **item, *tmp;
	nls_node *node = nls_new(nls_node);

	if (!node) {
		return 1;
	}
	node->nn_type = NLS_TYPE_INT;
	node->nn_int = 0;
	nls_list_foreach(arg, &item, &tmp) {
		node->nn_int += (*item)->nn_int;
	}
	*out = node;
	return 0;
}

int
nls_func_sub(nls_node *arg, nls_node **out)
{
	nls_node **item, *tmp;
	nls_node *node = nls_new(nls_node);

	if (!node) {
		return 1;
	}
	node->nn_type = NLS_TYPE_INT;
	node->nn_int = 0;
	nls_list_foreach(arg, &item, &tmp) {
		if (!node->nn_int) {
			node->nn_int = (*item)->nn_int;
		} else {
			node->nn_int -= (*item)->nn_int;
		}
	}
	*out = node;
	return 0;
}

int
nls_func_mul(nls_node *arg, nls_node **out)
{
	nls_node **item, *tmp;
	nls_node *node = nls_new(nls_node);

	if (!node) {
		return 1;
	}
	node->nn_type = NLS_TYPE_INT;
	node->nn_int = 0;
	nls_list_foreach(arg, &item, &tmp) {
		if (!node->nn_int) {
			node->nn_int = (*item)->nn_int;
		} else {
			node->nn_int *= (*item)->nn_int;
		}
	}
	*out = node;
	return 0;
}

int
nls_func_div(nls_node *arg, nls_node **out)
{
	nls_node **item, *tmp;
	nls_node *node = nls_new(nls_node);

	if (!node) {
		return 1;
	}
	node->nn_type = NLS_TYPE_INT;
	node->nn_int = 0;
	nls_list_foreach(arg, &item, &tmp) {
		if (!node->nn_int) {
			node->nn_int = (*item)->nn_int;
		} else {
			node->nn_int /= (*item)->nn_int;
		}
	}
	*out = node;
	return 0;
}

int
nls_func_mod(nls_node *arg, nls_node **out)
{
	nls_node **item, *tmp;
	nls_node *node = nls_new(nls_node);

	if (!node) {
		return 1;
	}
	node->nn_type = NLS_TYPE_INT;
	node->nn_int = 0;
	nls_list_foreach(arg, &item, &tmp) {
		if (!node->nn_int) {
			node->nn_int = (*item)->nn_int;
		} else {
			node->nn_int %= (*item)->nn_int;
		}
	}
	*out = node;
	return 0;
}
