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
#define NLS_MSG_NO_SUCH_SYMBOL "No such symbol"

#define NLS_ANON_VAR_NAME_BUF_SIZE 32
#define NLS_ANON_VAR_PREFIX 'x'

NLS_GLOBAL FILE *nls_sys_out;
NLS_GLOBAL FILE *nls_sys_err;
static nls_hash nls_sym_table;

static void nls_sym_table_init(void);
static void nls_sym_table_term(void);
static int nls_apply_function(nls_node *func, nls_node *args, nls_node **out);
static int nls_part_apply_function(nls_node *func, nls_node *args, nls_node **out);
static int nls_apply_abstraction(nls_node **func, nls_node *args);
static void nls_replace_vars(nls_node *vars, nls_node *args);
static void nls_remove_head_vars(nls_node *func, int n);
static nls_node* nls_vars_new(int n);

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
		if ((ret = nls_apply(item))) {
			NLS_ERROR(NLS_MSG_REDUCTION_FAIL ": errno=%d: %s",
				ret, strerror(ret));
			goto free_exit;
		}
		nls_node_print(*item, nls_sys_out);
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
 * [DESTRUCTIVE] Do lambda application.
 * @param  tree Target syntax tree.
 * @retval 0    Application succeed.
 * @retval else Error code.
 */
int
nls_apply(nls_node **tree)
{
	int ret;
	nls_node *out, *tmp;

	nls_application *app = &((*tree)->nn_app);
	nls_node **func = &(app->nap_func);
	nls_node **args = &(app->nap_args);

	if (NLS_ISVAR(*tree)) {
		if (!(out = nls_symbol_get((*tree)->nn_var.nv_name))) {
			return 0;
		}
		nls_release(*tree);
		*tree = nls_grab(out);
		return 0;
	}
	if (!NLS_ISAPP(*tree)) {
		return 0;
	}
	switch ((*func)->nn_type) {
	case NLS_TYPE_VAR:
		if (!(tmp = nls_symbol_get((*func)->nn_var.nv_name))) {
			NLS_ERROR(NLS_MSG_NO_SUCH_SYMBOL ": %s",
				(*func)->nn_var.nv_name->ns_bufp);
			return EINVAL;
		}
		nls_release(*func);
		*func = nls_grab(nls_node_clone(tmp));
		return nls_apply(tree);
	case NLS_TYPE_FUNCTION:
		if ((ret = nls_apply_function(*func, *args, &out))) {
			return ret;
		}
		out = nls_grab(out);
		nls_release(*tree);
		*tree = out;
		break;
	case NLS_TYPE_ABSTRACTION:
		if ((ret = nls_apply_abstraction(func, *args))) {
			return ret;
		}
		out = nls_grab(*func);
		nls_release(*tree);
		*tree = out;
		break;
	case NLS_TYPE_APPLICATION:
		if ((ret = nls_apply(func))) {
			return ret;
		}
		return nls_apply(tree);
	default:
		NLS_BUG(NLS_MSG_INVALID_NODE_TYPE
			": type=%d", (*func)->nn_type);
	}
	return 0;
}

nls_node*
nls_symbol_get(nls_string *name)
{
	nls_hash_entry *ent, *prev;

	ent = nls_hash_search(&nls_sym_table, name, &prev);
	if (!ent) {
		return NULL;
	}
	return ent->nhe_node;
}

void
nls_symbol_set(nls_string *name, nls_node *node)
{
	nls_hash_add(&nls_sym_table, name, node);
}

static void
nls_sym_table_init(void)
{
	nls_hash_init(&nls_sym_table);
#define NLS_SYM_TABLE_ADD_FUNC(fp, n, name) \
	do { \
		nls_node *func = nls_function_new((fp), (n), (name)); \
		if (!func) { \
			NLS_ERROR(NLS_MSG_ENOMEM); \
			return; \
		} \
		nls_symbol_set(func->nn_func.nf_name, func); \
	} while(0)
	NLS_SYM_TABLE_ADD_FUNC(nls_func_add,  2, "add");
	NLS_SYM_TABLE_ADD_FUNC(nls_func_sub,  2, "sub");
	NLS_SYM_TABLE_ADD_FUNC(nls_func_mul,  2, "mul");
	NLS_SYM_TABLE_ADD_FUNC(nls_func_div,  2, "div");
	NLS_SYM_TABLE_ADD_FUNC(nls_func_mod,  2, "mod");
	NLS_SYM_TABLE_ADD_FUNC(nls_func_abst, 2, "abst");
	NLS_SYM_TABLE_ADD_FUNC(nls_func_set,  2, "set");
#undef  NLS_SYM_TABLE_ADD_FUNC
}

static void
nls_sym_table_term(void)
{
	nls_hash_term(&nls_sym_table);
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
	if ((ret = nls_apply(func))) {
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
