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
#include <stdio.h>
#include "nameless.h"
#include "nameless/node.h"
#include "nameless/mm.h"

#define NLS_ANON_VAR_NAME_BUF_SIZE 32
#define NLS_ANON_VAR_PREFIX 'x'

#define NLS_MSG_BROKEN_LIST "Broken list"
#define NLS_MSG_NO_SUCH_SYMBOL "No such symbol"

#define NLS_TYPE_int		NLS_TYPE_INT
#define NLS_TYPE_var		NLS_TYPE_VAR
#define NLS_TYPE_function	NLS_TYPE_FUNCTION
#define NLS_TYPE_abstraction	NLS_TYPE_ABSTRACTION
#define NLS_TYPE_application	NLS_TYPE_APPLICATION
#define NLS_TYPE_list		NLS_TYPE_LIST

#define NLS_NODE_NEW(type) \
	_nls_node_new(NLS_TYPE_##type, &nls_##type##_operations)

#define NLS_DEF_NODE_OPERATIONS(type) \
	static nls_node_operations nls_##type##_operations = { \
		.nop_release = nls_##type##_release, \
		.nop_clone   = nls_##type##_clone, \
		.nop_print   = nls_##type##_print, \
		.nop_apply   = nls_##type##_apply, \
	}

static nls_node* _nls_node_new(nls_node_type_t type, nls_node_operations *op);
static void nls_list_item_free(nls_node *node);
static nls_node* nls_list_tail_entry(nls_node *node);
static void nls_register_vars(nls_node **tree, nls_node *var);

static void nls_int_release(nls_node *tree);
static void nls_var_release(nls_node *tree);
static void nls_function_release(nls_node *tree);
static void nls_abstraction_release(nls_node *tree);
static void nls_application_release(nls_node *tree);
static void nls_list_release(nls_node *tree);

static nls_node* nls_int_clone(nls_node *tree);
static nls_node* nls_var_clone(nls_node *tree);
static nls_node* nls_function_clone(nls_node *tree);
static nls_node* nls_abstraction_clone(nls_node *tree);
static nls_node* nls_application_clone(nls_node *tree);
static nls_node* nls_list_clone(nls_node *tree);

static void nls_int_print(nls_node *node, FILE* out);
static void nls_var_print(nls_node *node, FILE* out);
static void nls_function_print(nls_node *node, FILE* out);
static void nls_abstraction_print(nls_node *node, FILE* out);
static void nls_application_print(nls_node *node, FILE* out);
static void nls_list_print(nls_node *node, FILE* out);

static int nls_int_apply(nls_node **tree);
static int nls_var_apply(nls_node **tree);
static int nls_function_apply(nls_node **tree);
static int nls_abstraction_apply(nls_node **tree);
static int nls_application_apply(nls_node **tree);
static int nls_list_apply(nls_node **tree);

static int nls_part_apply_function(nls_node *func, nls_node *args, nls_node **out);
static int nls_apply_abstraction(nls_node **func, nls_node *args);
static void nls_replace_vars(nls_node *vars, nls_node *args);
static void nls_remove_head_vars(nls_node *func, int n);
static nls_node* nls_vars_new(int n);

NLS_DEF_NODE_OPERATIONS(int);
NLS_DEF_NODE_OPERATIONS(var);
NLS_DEF_NODE_OPERATIONS(function);
NLS_DEF_NODE_OPERATIONS(abstraction);
NLS_DEF_NODE_OPERATIONS(application);
NLS_DEF_NODE_OPERATIONS(list);

void
nls_node_free(void *ptr)
{
	nls_node *node = (nls_node*)ptr;

	node->nn_op->nop_release(node);
	nls_free(node);
}

nls_node*
nls_int_new(int val)
{
	nls_node *node = NLS_NODE_NEW(int);

	if (!node) {
		return NULL;
	}
	node->nn_int = val;
	return node;
}

nls_node*
nls_var_new(nls_string *name)
{
	nls_node *node = NLS_NODE_NEW(var);

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
	node = NLS_NODE_NEW(function);
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
	int i, n = nls_list_count(vars);
	nls_abstraction *abst;
	nls_node *node;
	nls_node **var, *tmp;

	if (!(node = NLS_NODE_NEW(abstraction))) {
		return NULL;
	}

	i = 0;
	nls_list_foreach(vars, &var, &tmp) {
		nls_register_vars(&def, *var);
	}
	abst = &(node->nn_abst);
	abst->nab_num_args = n;
	abst->nab_vars = nls_grab(vars);
	abst->nab_def  = nls_grab(def);
	return node;
}

nls_node*
nls_application_new(nls_node *func, nls_node *args)
{
	nls_application *app;
	nls_node *node = NLS_NODE_NEW(application);

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
	nls_node *node = NLS_NODE_NEW(list);

	if (!node) {
		return NULL;
	}
	list = &(node->nn_list);
	list->nl_head = nls_grab(item);
	list->nl_rest = NULL;
	return node;
}

nls_node*
nls_node_clone(nls_node *tree)
{
	return tree->nn_op->nop_clone(tree);
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

static void
nls_register_vars(nls_node **tree, nls_node *var)
{
	switch ((*tree)->nn_type) {
	case NLS_TYPE_INT:
		return;
	case NLS_TYPE_VAR:
		if (!nls_strcmp((*tree)->nn_var.nv_name, var->nn_var.nv_name)) {
			nls_node **next_ref = var->nn_var.nv_next_ref;

			(*tree)->nn_var.nv_next_ref = next_ref;
			var->nn_var.nv_next_ref = tree;
		}
		return;
	case NLS_TYPE_FUNCTION:
		return;
	case NLS_TYPE_ABSTRACTION:
		nls_register_vars(&((*tree)->nn_abst.nab_vars), var);
		nls_register_vars(&((*tree)->nn_abst.nab_def),  var);
		return;
	case NLS_TYPE_APPLICATION:
		nls_register_vars(&((*tree)->nn_app.nap_args), var);
		return;
	case NLS_TYPE_LIST:
		{
			nls_node **item, *tmp;

			nls_list_foreach((*tree), &item, &tmp) {
				nls_register_vars(item, var);
			}
		}
		return;
	default:
		NLS_BUG(NLS_MSG_INVALID_NODE_TYPE);
		return;
	}
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

static nls_node*
nls_int_clone(nls_node *tree)
{
	return nls_int_new(tree->nn_int);
}

static nls_node*
nls_var_clone(nls_node *tree)
{
	nls_string *str = nls_string_new(tree->nn_var.nv_name->ns_bufp);

	if (!str) {
		NLS_ERROR(NLS_MSG_ENOMEM);
		return NULL;
	}
	return nls_var_new(str);
}

static nls_node*
nls_function_clone(nls_node *tree)
{
	nls_function *func = &(tree->nn_func);

	return nls_function_new(func->nf_fp,
		func->nf_num_args, func->nf_name->ns_bufp);
}

static nls_node*
nls_abstraction_clone(nls_node *tree)
{
	nls_node *vars, *def;
	nls_abstraction *abst = &(tree->nn_abst);

	if (!(vars = nls_node_clone(abst->nab_vars))) {
		NLS_ERROR(NLS_MSG_ENOMEM);
		return NULL;
	}
	if (!(def = nls_node_clone(abst->nab_def))) {
		NLS_ERROR(NLS_MSG_ENOMEM);
		return NULL;
	}
	return nls_abstraction_new(vars, def);
}

static nls_node*
nls_application_clone(nls_node *tree)
{
	nls_node *func, *args;
	nls_application *app = &(tree->nn_app);

	if (!(func = nls_node_clone(app->nap_func))) {
		NLS_ERROR(NLS_MSG_ENOMEM);
		return NULL;
	}
	if (!(args = nls_node_clone(app->nap_args))) {
		NLS_ERROR(NLS_MSG_ENOMEM);
		return NULL;
	}
	return nls_application_new(func, args);
}

static nls_node*
nls_list_clone(nls_node *tree)
{
	int first = 1;
	nls_node *new;
	nls_node **item, *tmp;

	nls_list_foreach(tree, &item, &tmp) {
		nls_node *clone = nls_node_clone(*item);

		if (!clone) {
			NLS_ERROR(NLS_MSG_ENOMEM);
			return NULL;
		}
		if (!first) {
			nls_list_add(new, clone);
			continue;
		}
		if (first) {
			first = 0;
		}
		if (!(new = nls_list_new(clone))) {
			NLS_ERROR(NLS_MSG_ENOMEM);
			return NULL;
		}
	}
	return new;
}

void
nls_node_print(nls_node *node, FILE* out)
{
	(node->nn_op->nop_print)(node, out);
}

static void
nls_int_print(nls_node *node, FILE* out)
{
	fprintf(out, "%d", node->nn_int);
}

static void
nls_var_print(nls_node *node, FILE* out)
{
	fprintf(out, "%s", node->nn_var.nv_name->ns_bufp);
}

static void
nls_function_print(nls_node *node, FILE* out)
{
	fprintf(out, "%s", node->nn_func.nf_name->ns_bufp);
}

static void
nls_abstraction_print(nls_node *node, FILE* out)
{
	fprintf(out, "lambda");
	nls_node_print(node->nn_abst.nab_vars, out);
	fprintf(out, ".");
	nls_node_print(node->nn_abst.nab_def, out);
}

static void
nls_application_print(nls_node *node, FILE* out)
{
	nls_node_print(node->nn_app.nap_func, out);
	nls_node_print(node->nn_app.nap_args, out);
}

static void
nls_list_print(nls_node *node, FILE* out)
{
	int first = 1;
	nls_node **item, *tmp;

	fprintf(out, "(");
	nls_list_foreach(node, &item, &tmp) {
		if (!first) {
			fprintf(out, " ");
		}
		if (first) {
			first = 0;
		}
		nls_node_print(*item, out);
	}
	fprintf(out, ")");
}

static int
nls_int_apply(nls_node **tree)
{
	/* Nothing to do. */
	return 0;
}

static int
nls_var_apply(nls_node **tree)
{
	nls_node *tmp;
	nls_application *app = &((*tree)->nn_app);
	nls_node **func = &(app->nap_func);

	if (!(tmp = nls_symbol_get((*func)->nn_var.nv_name))) {
		NLS_ERROR(NLS_MSG_NO_SUCH_SYMBOL ": %s",
			(*func)->nn_var.nv_name->ns_bufp);
		return EINVAL;
	}
	nls_release(*func);
	*func = nls_grab(nls_node_clone(tmp));
	return nls_eval(tree);
}

static int
nls_function_apply(nls_node **tree)
{
	int ret;
	nls_node *out;

	nls_application *app = &((*tree)->nn_app);
	nls_node *func = app->nap_func;
	nls_node *args = app->nap_args;

	nls_fp fp = func->nn_func.nf_fp;
	int func_nargs = func->nn_func.nf_num_args;
	int num_args = nls_list_count(args);

	if (num_args > func_nargs) {
		NLS_ERROR(NLS_MSG_TOO_MANY_ARGS " expected=%d actual=%d",
			func_nargs, num_args);
		return EINVAL;
	}
	if (num_args < func_nargs) {
		if ((ret = nls_part_apply_function(func, args, &out))) {
			return ret;
		}
		goto set_result_exit;
	}
	if ((ret = (fp)(args, &out))) {
		return ret;
	}
set_result_exit:
	out = nls_grab(out);
	nls_release(*tree);
	*tree = out;
	return 0;
}

static int
nls_abstraction_apply(nls_node **tree)
{
	int ret;
	nls_node *out;

	nls_application *app = &((*tree)->nn_app);
	nls_node **func = &(app->nap_func);
	nls_node **args = &(app->nap_args);

	if ((ret = nls_apply_abstraction(func, *args))) {
		return ret;
	}
	out = nls_grab(*func);
	nls_release(*tree);
	*tree = out;
	return 0;
}

static int
nls_application_apply(nls_node **tree)
{
	int ret;
	nls_application *app = &((*tree)->nn_app);
	nls_node **func = &(app->nap_func);

	if ((ret = nls_eval(func))) {
		return ret;
	}
	return nls_eval(tree);
}

static int
nls_list_apply(nls_node **tree)
{
	/* Nothing to do. */
	return 0;
}

static int
nls_part_apply_function(nls_node *func, nls_node *args, nls_node **out)
{
	int num_args     = func->nn_func.nf_num_args;
	nls_string *name = func->nn_func.nf_name;
	nls_fp fp        = func->nn_func.nf_fp;

	nls_node *vars, *add_vars;
	nls_node *def, *curry;
	int num_lack = num_args - nls_list_count(args);

	vars = nls_vars_new(num_lack);
	if (!vars) {
		return ENOMEM;
	}
	add_vars = nls_vars_new(num_lack);
	if (!add_vars) {
		nls_node_free(vars);
		return ENOMEM;
	}
	func = nls_function_new(fp, num_args, name->ns_bufp);
	if (!func) {
		nls_node_free(vars);
		nls_node_free(add_vars);
		return ENOMEM;
	}
	nls_list_concat(args, add_vars);
	def = nls_application_new(func, args);
	if (!def) {
		nls_node_free(func);
		nls_node_free(vars);
		return ENOMEM;
	}
	curry = nls_abstraction_new(vars, def);
	if (!curry) {
		nls_node_free(def);
		nls_node_free(vars);
		return ENOMEM;
	}
	*out = curry;
	return 0;
}

static int
nls_apply_abstraction(nls_node **func, nls_node *args)
{
	int ret;
	int num_args = nls_list_count(args);

	nls_node *out;
	nls_abstraction *abst = &((*func)->nn_abst);
	nls_node **vars = &(abst->nab_vars);
	int func_nargs  = abst->nab_num_args;

	if (num_args > func_nargs) {
		NLS_ERROR(NLS_MSG_TOO_MANY_ARGS " expected=%d actual=%d",
			func_nargs, num_args);
		return EINVAL;
	}

	nls_replace_vars(*vars, args);
	if (num_args < func_nargs) {
		/* Partial apply */
		nls_remove_head_vars(*func, num_args);
		return 0;
	}
	out = nls_grab(abst->nab_def);
	nls_release(*func);
	*func = out;
	if ((ret = nls_eval(func))) {
		return ret;
	}
	return 0;
}

static void
nls_replace_vars(nls_node *vars, nls_node *args)
{
	nls_node **var, **arg, *tmp1, *tmp2;

	nls_double_list_foreach(vars, args, &var, &arg, &tmp1, &tmp2) {
		nls_node **replace = (*var)->nn_var.nv_next_ref;
		nls_node **next = replace ?(*replace)->nn_var.nv_next_ref : NULL;

		while (replace) {
			nls_release(*replace);
			*replace = nls_grab(*arg);
			replace = next;
			next = next ? (*next)->nn_var.nv_next_ref : NULL;
		}
	}
}

static void
nls_remove_head_vars(nls_node *func, int n)
{
	int i;
	nls_abstraction *abst = &(func->nn_abst);
	nls_node **vars = &(abst->nab_vars);

	for (i = 0; i < n; i++) {
		nls_list_remove(vars);
	}
	abst->nab_num_args -= n;
}

static nls_node*
nls_vars_new(int n)
{
	int i;
	char *bufp, buf[NLS_ANON_VAR_NAME_BUF_SIZE];
	nls_node *var;
	nls_node *vars = NULL;

	buf[0] = NLS_ANON_VAR_PREFIX;
	bufp = buf + 1;
	for (i = 1; i <= n; i++) {
		nls_string *name;

		snprintf(bufp, NLS_ANON_VAR_NAME_BUF_SIZE-1, "%d", i);
		name = nls_string_new(buf);
		if (!name) {
			goto free_exit;
		}
		var = nls_var_new(name);
		if (!var) {
			nls_string_free(name);
			goto free_exit;
		}
		if (!vars) {
			vars = nls_list_new(var);
			if (!vars) {
				nls_node_free(var);
				return NULL;
			}
			continue;
		}
		nls_list_add(vars, var);
	}
	return vars;
free_exit:
	if (vars) {
		nls_node_free(vars);
	}
	return NULL;
}
