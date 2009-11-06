#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "y.tab.h"
#include "nameless.h"
#include "nameless/parser.h"
#include "nameless/mm.h"
#include "nameless/hash.h"
#include "nameless/function.h"

NLS_GLOBAL FILE *nls_sys_out;
NLS_GLOBAL FILE *nls_sys_err;
NLS_GLOBAL nls_hash nls_sys_sym_table;

static void nls_sym_table_init(void);
static void nls_sym_table_term(void);
static int nls_list_reduce(nls_node **tree);
static int nls_apply(nls_node **node);
static int nls_apply_abstraction(nls_node **func, nls_node *args);
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
	ret = nls_reduce(&tree);
	nls_list_foreach(tree, &item, &tmp) {
		nls_tree_print(nls_sys_out, *item);
		fprintf(out, "\n");
	}
free_exit:
	if (tree) {
		nls_node_release(tree);
	}
	nls_term();
	return ret;
}

void
nls_init(FILE *out, FILE *err)
{
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
		return nls_list_reduce(tree);
	default:
		NLS_BUG(NLS_MSG_INVALID_NODE_TYPE ": type=%d",
			(*tree)->nn_type);
		return EINVAL; /* must not happen */
	}
}

static void
nls_sym_table_init(void)
{
	nls_hash_init(&nls_sys_sym_table);
#define NLS_SYM_TABLE_ADD_FUNC(fp, name) \
	do { \
		nls_node *func = nls_function_new((fp), (name)); \
		nls_hash_add(&nls_sys_sym_table, func->nn_func.nf_name, func); \
	} while(0)
	NLS_SYM_TABLE_ADD_FUNC(nls_func_lambda , "lambda");
	NLS_SYM_TABLE_ADD_FUNC(nls_func_add , "+");
	NLS_SYM_TABLE_ADD_FUNC(nls_func_sub , "-");
	NLS_SYM_TABLE_ADD_FUNC(nls_func_mul , "*");
	NLS_SYM_TABLE_ADD_FUNC(nls_func_div , "/");
	NLS_SYM_TABLE_ADD_FUNC(nls_func_mod , "%");
#undef  NLS_SYM_TABLE_ADD_FUNC
}

static void
nls_sym_table_term(void)
{
	nls_hash_term(&nls_sys_sym_table);
}

static int
nls_list_reduce(nls_node **list)
{
	int ret;
	nls_node **item, *tmp;

	nls_list_foreach(*list, &item, &tmp) {
		ret = nls_reduce(item);
		if (ret) {
			return ret;
		}
	}
	return 0;
}

/*
 * @return positive or 0: evaluation result
 * 	   negative	: error code
 */
static int
nls_apply(nls_node **node)
{
	int ret;
	nls_application *app;
	nls_node **func, **args;

retry:
	app  = &((*node)->nn_app);
	func = &(app->nap_func);
	args = &(app->nap_args);

	switch ((*func)->nn_type) {
	case NLS_TYPE_FUNCTION:
		{
			nls_node *out;
			nls_fp fp = (*func)->nn_func.nf_fp;

			if ((ret = (fp)(*args, &out))) {
				return ret;
			}
			out = nls_node_grab(out);
			nls_node_release(*node);
			*node = out;
		}
		break;
	case NLS_TYPE_ABSTRACTION:
		{
			nls_node *out;

			if ((ret = nls_apply_abstraction(func, *args))) {
				return ret;
			}
			out = nls_node_grab(*func);
			nls_node_release(*node);
			*node = out;
		}
		break;
	case NLS_TYPE_APPLICATION:
		if ((ret = nls_reduce(func))) {
			return ret;
		}
		goto retry;
	default:
		NLS_BUG(NLS_MSG_INVALID_NODE_TYPE
			": type=%d", (*func)->nn_type);
	}
	return 0;
}

static int
nls_apply_abstraction(nls_node **func, nls_node *args)
{
	int ret;

	nls_node *out;
	nls_abstraction *abst = &((*func)->nn_abst);
	nls_node *vars = abst->nab_vars;

	nls_node **var, **arg, *tmp1, *tmp2;
	nls_double_list_foreach(vars, args, &var, &arg, &tmp1, &tmp2) {
		nls_node **replace, **next;

		for (replace = (*var)->nn_var.nv_next_ref,
			next = replace ? (*replace)->nn_var.nv_next_ref : NULL;
			replace;
			replace = next,
			next = next ? (*next)->nn_var.nv_next_ref : NULL) {
			nls_node_release(*replace);
			*replace = nls_node_grab(*arg);
		}
	}
	out = nls_node_grab(abst->nab_def);
	nls_node_release(*func);
	*func = out;
	if ((ret = nls_reduce(func))) {
		return ret;
	}
	return 0;
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
		fprintf(out, "(lambda");
		if ((ret = nls_tree_print(out, tree->nn_abst.nab_vars))) {
			return ret;
		}
		fprintf(out, " ");
		if ((ret = nls_tree_print(out, tree->nn_abst.nab_def))) {
			return ret;
		}
		fprintf(out, ")");
		return 0;
	case NLS_TYPE_APPLICATION:
		fprintf(out, "(");
		if ((ret = nls_tree_print(out, tree->nn_app.nap_func))) {
			return ret;
		}
		fprintf(out, " ");
		if ((ret = nls_tree_print(out, tree->nn_app.nap_args))) {
			return ret;
		}
		fprintf(out, ")");
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
