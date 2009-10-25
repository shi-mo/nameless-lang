#include "nameless.h"

int
nls_func_add(nls_node *arg, nls_node **out)
{
	nls_node **tmp;
	nls_node *node = nls_new(nls_node);

	if (!node) {
		return 1;
	}
	node->nn_type = NLS_TYPE_INT;
	node->nn_u.nnu_int = 0;
	nls_list_foreach(tmp, arg) {
		node->nn_u.nnu_int += (*tmp)->nn_u.nnu_int;
	}
	*out = node;
	return 0;
}

int
nls_func_sub(nls_node *arg, nls_node **out)
{
	nls_node **tmp;
	nls_node *node = nls_new(nls_node);

	if (!node) {
		return 1;
	}
	node->nn_type = NLS_TYPE_INT;
	node->nn_u.nnu_int = 0;
	nls_list_foreach(tmp, arg) {
		if (!node->nn_u.nnu_int) {
			node->nn_u.nnu_int = (*tmp)->nn_u.nnu_int;
		} else {
			node->nn_u.nnu_int -= (*tmp)->nn_u.nnu_int;
		}
	}
	*out = node;
	return 0;
}

int
nls_func_mul(nls_node *arg, nls_node **out)
{
	nls_node **tmp;
	nls_node *node = nls_new(nls_node);

	if (!node) {
		return 1;
	}
	node->nn_type = NLS_TYPE_INT;
	node->nn_u.nnu_int = 0;
	nls_list_foreach(tmp, arg) {
		if (!node->nn_u.nnu_int) {
			node->nn_u.nnu_int = (*tmp)->nn_u.nnu_int;
		} else {
			node->nn_u.nnu_int *= (*tmp)->nn_u.nnu_int;
		}
	}
	*out = node;
	return 0;
}

int
nls_func_div(nls_node *arg, nls_node **out)
{
	nls_node **tmp;
	nls_node *node = nls_new(nls_node);

	if (!node) {
		return 1;
	}
	node->nn_type = NLS_TYPE_INT;
	node->nn_u.nnu_int = 0;
	nls_list_foreach(tmp, arg) {
		if (!node->nn_u.nnu_int) {
			node->nn_u.nnu_int = (*tmp)->nn_u.nnu_int;
		} else {
			node->nn_u.nnu_int /= (*tmp)->nn_u.nnu_int;
		}
	}
	*out = node;
	return 0;
}

int
nls_func_mod(nls_node *arg, nls_node **out)
{
	nls_node **tmp;
	nls_node *node = nls_new(nls_node);

	if (!node) {
		return 1;
	}
	node->nn_type = NLS_TYPE_INT;
	node->nn_u.nnu_int = 0;
	nls_list_foreach(tmp, arg) {
		if (!node->nn_u.nnu_int) {
			node->nn_u.nnu_int = (*tmp)->nn_u.nnu_int;
		} else {
			node->nn_u.nnu_int %= (*tmp)->nn_u.nnu_int;
		}
	}
	*out = node;
	return 0;
}
