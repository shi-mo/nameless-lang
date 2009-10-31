#include <stdio.h>
#include <errno.h>
#include "y.tab.h"
#include "nameless.h"
#include "nameless/parser.h"
#include "nameless/mm.h"

NLS_GLOBAL FILE *nls_sys_out;
NLS_GLOBAL FILE *nls_sys_err;

static int nls_list_reduce(nls_node **tree);
static int nls_apply(nls_node **node);
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

	nls_mem_chain_init();
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
		nls_tree_free(tree);
	}
	nls_mem_chain_term();
	return ret;
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

void
nls_string_free(nls_string *str)
{
	nls_release(str->ns_bufp);
	nls_release(str);
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

	nls_application *app = &((*node)->nn_app);
	nls_node *func = app->na_func;
	nls_node **arg = &app->na_arg;

	switch (func->nn_type) {
	case NLS_TYPE_FUNCTION:
		{
			nls_node *out;
			nls_function fp = func->nn_func;

			if ((ret = (fp)(*arg, &out))) {
				return ret;
			}
			out = nls_node_grab(out);
			nls_tree_free(*node);
			*node = out;
		}
		break;
	case NLS_TYPE_ABSTRACTION:
		NLS_ERROR(NLS_MSG_NOT_IMPLEMENTED);
		break;
	default:
		NLS_BUG(NLS_MSG_INVALID_NODE_TYPE
			": type=%d", func->nn_type);
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
		fprintf(out, "func:%p", tree->nn_func);
		return 0;
	case NLS_TYPE_ABSTRACTION:
		fprintf(out, "(abst(%d): ", tree->nn_abst.nab_num_arg);
		if ((ret = nls_tree_print(out, tree->nn_abst.nab_def))) {
			return ret;
		}
		fprintf(out, ")");
		return 0;
	case NLS_TYPE_APPLICATION:
		fprintf(out, "(");
		if ((ret = nls_tree_print(out, tree->nn_app.na_func))) {
			return ret;
		}
		fprintf(out, " ");
		if ((ret = nls_tree_print(out, tree->nn_app.na_arg))) {
			return ret;
		}
		fprintf(out, ")");
		return 0;
	case NLS_TYPE_LIST:
		{
			int first = 1;
			nls_node **item, *tmp;

			fprintf(out, "[");
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
			fprintf(out, "]");
		}
		return 0;
	default:
		NLS_BUG(NLS_MSG_INVALID_NODE_TYPE ": type=%d", tree->nn_type);
		return EINVAL;
	}
}
