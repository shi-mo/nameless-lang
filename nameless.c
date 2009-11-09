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
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "y.tab.h"
#include "nameless.h"
#include "nameless/parser.h"
#include "nameless/mm.h"
#include "nameless/hash.h"
#include "nameless/function.h"

#define NLS_MSG_REDUCTION_FAIL "Reduction failure"

#define NLS_ANON_VAR_NAME_BUF_SIZE 32
#define NLS_ANON_VAR_PREFIX 'x'

NLS_GLOBAL FILE *nls_sys_out;
NLS_GLOBAL FILE *nls_sys_err;
NLS_GLOBAL nls_hash nls_sys_sym_table;

static void nls_sym_table_init(void);
static void nls_sym_table_term(void);
static int nls_apply(nls_node **node);
static int nls_apply_function(nls_node *func, nls_node *args, nls_node **out);
static int nls_part_apply_function(nls_node *func, nls_node *args, nls_node **out);
static int nls_apply_abstraction(nls_node **func, nls_node *args);
static void nls_replace_vars(nls_node *vars, nls_node *args);
static void nls_remove_head_vars(nls_node *func, int n);
static nls_node* nls_vars_new(int n);
static int nls_tree_print(FILE *out, nls_node *tree);

int
nls_main(FILE *in, FILE *out, FILE *err)
{
	int ret;
	nls_node *tree;
	nls_node **item, *tmp;

	yyin  = in;
	yyout = out;
	nls_sys_out = out;
	nls_sys_err = err;

	nls_init(out, err);
	ret = yyparse();
	tree = nls_sys_parse_result; /* pointer grabbed in yyparse(). */
	if (ret || !tree) {
		goto free_exit;
	}
	nls_list_foreach(tree, &item, &tmp) {
		if ((ret = nls_reduce(item))) {
			NLS_ERROR(NLS_MSG_REDUCTION_FAIL ": errno=%d: %s",
				ret, strerror(ret));
			goto free_exit;
		}
		nls_tree_print(nls_sys_out, *item);
		fprintf(out, "\n");
	}
free_exit:
	if (tree) {
		nls_release(tree);
	}
	nls_term();
	return ret;
}

void
nls_init(FILE *out, FILE *err)
{
#if YYDEBUG
	yydebug = 1;
#endif /* YYDEBUG */
	nls_sys_out = out;
	nls_sys_err = err;
	nls_mem_chain_init();
	nls_sym_table_init();
}

void
nls_term(void)
{
	nls_sym_table_term();
	nls_mem_chain_term();
}

/**
 * [DESTRUCTIVE] Recursively do application.
 * @param  tree Target syntax tree.
 * @retval 0    Application succeed.
 * @retval else Error code.
 */
int
nls_reduce(nls_node **tree)
{
	int ret;

	switch ((*tree)->nn_type) {
	case NLS_TYPE_INT:
	case NLS_TYPE_VAR:
	case NLS_TYPE_FUNCTION:
	case NLS_TYPE_ABSTRACTION:
		return 0;
	case NLS_TYPE_APPLICATION:
		if ((ret = nls_apply(tree))) {
			return ret;
		}
		return nls_reduce(tree);
	case NLS_TYPE_LIST:
		return 0;
	default:
		NLS_BUG(NLS_MSG_INVALID_NODE_TYPE ": type=%d",
			(*tree)->nn_type);
		return EINVAL; /* must not happen */
	}
}

nls_node*
nls_symbol_get(nls_string *name)
{
	nls_hash_entry *ent, *prev;

	ent = nls_hash_search(&nls_sys_sym_table, name, &prev);
	if (!ent) {
		return NULL;
	}
	return ent->nhe_node;
}

static void
nls_sym_table_init(void)
{
	nls_hash_init(&nls_sys_sym_table);
#define NLS_SYM_TABLE_ADD_FUNC(fp, n, name) \
	do { \
		nls_node *func = nls_function_new((fp), (n), (name)); \
		if (!func) { \
			NLS_ERROR(NLS_MSG_FAILED_TO_ALLOCATE_MEMORY); \
			return; \
		} \
		nls_hash_add(&nls_sys_sym_table, func->nn_func.nf_name, func); \
	} while(0)
	NLS_SYM_TABLE_ADD_FUNC(nls_func_abst, 2, "abst");
	NLS_SYM_TABLE_ADD_FUNC(nls_func_add,  2, "add");
	NLS_SYM_TABLE_ADD_FUNC(nls_func_sub,  2, "sub");
	NLS_SYM_TABLE_ADD_FUNC(nls_func_mul,  2, "mul");
	NLS_SYM_TABLE_ADD_FUNC(nls_func_div,  2, "div");
	NLS_SYM_TABLE_ADD_FUNC(nls_func_mod,  2, "mod");
#undef  NLS_SYM_TABLE_ADD_FUNC
}

static void
nls_sym_table_term(void)
{
	nls_hash_term(&nls_sys_sym_table);
}

/*
 * @return 0  evaluation result
 * 	   !0 error code
 */
static int
nls_apply(nls_node **node)
{
	int ret;
	nls_node *out;

	nls_application *app = &((*node)->nn_app);
	nls_node **func = &(app->nap_func);
	nls_node **args = &(app->nap_args);

	switch ((*func)->nn_type) {
	case NLS_TYPE_FUNCTION:
		if ((ret = nls_apply_function(*func, *args, &out))) {
			return ret;
		}
		out = nls_node_grab(out);
		nls_release(*node);
		*node = out;
		break;
	case NLS_TYPE_ABSTRACTION:
		if ((ret = nls_apply_abstraction(func, *args))) {
			return ret;
		}
		out = nls_node_grab(*func);
		nls_release(*node);
		*node = out;
		break;
	case NLS_TYPE_APPLICATION:
		if ((ret = nls_reduce(func))) {
			return ret;
		}
		return nls_apply(node);
	default:
		NLS_BUG(NLS_MSG_INVALID_NODE_TYPE
			": type=%d", (*func)->nn_type);
	}
	return 0;
}

static int
nls_apply_function(nls_node *func, nls_node *args, nls_node **out)
{
	int ret;
	int num_args = nls_list_count(args);

	nls_fp fp = func->nn_func.nf_fp;
	int func_nargs = func->nn_func.nf_num_args;

	if (num_args > func_nargs) {
		NLS_ERROR(NLS_MSG_TOO_MANY_ARGS " expected=%d actual=%d",
			func_nargs, num_args);
		return EINVAL;
	}
	if (num_args < func_nargs) {
		return nls_part_apply_function(func, args, out);
	}
	if ((ret = (fp)(args, out))) {
		return ret;
	}
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
		nls_release(nls_node_grab(vars));
		return ENOMEM;
	}
	func = nls_function_new(fp, num_args, name->ns_bufp);
	if (!func) {
		nls_release(nls_node_grab(vars));
		nls_release(nls_node_grab(add_vars));
		return ENOMEM;
	}
	nls_list_concat(args, add_vars);
	def = nls_application_new(func, args);
	if (!def) {
		nls_release(nls_node_grab(func));
		nls_release(vars);
		return ENOMEM;
	}
	curry = nls_abstraction_new(vars, def);
	if (!curry) {
		nls_release(nls_node_grab(def));
		nls_release(vars);
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
	out = nls_node_grab(abst->nab_def);
	nls_release(*func);
	*func = out;
	if ((ret = nls_reduce(func))) {
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
			*replace = nls_node_grab(*arg);
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
				nls_release(nls_node_grab(var));
				return NULL;
			}
			continue;
		}
		nls_list_add(vars, var);
	}
	return vars;
free_exit:
	if (vars) {
		nls_release(nls_node_grab(vars));
	}
	return NULL;
}

static int
nls_tree_print(FILE *out, nls_node *tree)
{
	int ret;

	switch (tree->nn_type) {
	case NLS_TYPE_INT:
		fprintf(out, "%d", tree->nn_int);
		return 0;
	case NLS_TYPE_VAR:
		fprintf(out, "%s", tree->nn_var.nv_name->ns_bufp);
		return 0;
	case NLS_TYPE_FUNCTION:
		fprintf(out, "%s", tree->nn_func.nf_name->ns_bufp);
		return 0;
	case NLS_TYPE_ABSTRACTION:
		fprintf(out, "lambda");
		if ((ret = nls_tree_print(out, tree->nn_abst.nab_vars))) {
			return ret;
		}
		fprintf(out, ".");
		if ((ret = nls_tree_print(out, tree->nn_abst.nab_def))) {
			return ret;
		}
		return 0;
	case NLS_TYPE_APPLICATION:
		if ((ret = nls_tree_print(out, tree->nn_app.nap_func))) {
			return ret;
		}
		if ((ret = nls_tree_print(out, tree->nn_app.nap_args))) {
			return ret;
		}
		return 0;
	case NLS_TYPE_LIST:
		{
			int first = 1;
			nls_node **item, *tmp;

			fprintf(out, "(");
			nls_list_foreach(tree, &item, &tmp) {
				if (!first) {
					fprintf(out, " ");
				}
				if (first) {
					first = 0;
				}
				if ((ret = nls_tree_print(out, *item))) {
					return ret;
				}
			}
			fprintf(out, ")");
		}
		return 0;
	default:
		NLS_BUG(NLS_MSG_INVALID_NODE_TYPE ": type=%d", tree->nn_type);
		return EINVAL;
	}
}
