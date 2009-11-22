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

NLS_GLOBAL FILE *nls_sys_out;
NLS_GLOBAL FILE *nls_sys_err;
static nls_hash nls_sym_table;

static int nls_apply(nls_node **tree);
static void nls_sym_table_init(void);
static void nls_sym_table_term(void);

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
		if ((ret = nls_eval(item))) {
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
 * [DESTRUCTIVE] Evaluate an expression
 * @param  tree Target syntax tree.
 * @retval 0    Evaluation succeed.
 * @retval else Error code.
 */
int
nls_eval(nls_node **tree)
{
	nls_node *out;

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
	return nls_apply(tree);
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
nls_apply(nls_node **tree)
{
	nls_application *app = &((*tree)->nn_app);
	nls_node *func = app->nap_func;

	return ((func)->nn_op->nop_apply)(tree);
}
