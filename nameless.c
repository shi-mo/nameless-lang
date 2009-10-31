#include <stdio.h>
#include <errno.h>
#include "y.tab.h"
#include "nameless.h"
#include "nameless/parser.h"
#include "nameless/mm.h"

NLS_GLOBAL FILE *nls_sys_out;
NLS_GLOBAL FILE *nls_sys_err;

static int nls_reduce(nls_node **tree, int limit);
static int nls_list_reduce(nls_node **tree, int limit);
static int nls_apply(nls_node **node, int limit);
static void nls_list_item_free(nls_node *list);
static void nls_tree_free(nls_node *tree);
static int nls_tree_print(FILE *out, nls_node *tree);

int
nls_main(FILE *in, FILE *out, FILE *err)
{
	int ret;
	nls_node *tree = NULL;

	yyin  = in;
	yyout = out;
	nls_sys_out = out;
	nls_sys_err = err;

	nls_mem_chain_init();
	ret = yyparse();
	if (ret || !nls_sys_parse_result) {
		goto free_exit;
	}
	tree = nls_sys_parse_result;
	ret = nls_reduce(&tree, NLS_REDUCTION_LIMIT);
	nls_tree_print(nls_sys_out, tree);
	fprintf(out, "\n");
free_exit:
	if (tree) {
		nls_tree_free(tree);
	}
	nls_mem_chain_term();
	return ret;
}

/*
 * Recursively do application
 */
static int
nls_reduce(nls_node **tree, int limit)
{
	int ret;

	if (limit-- <= 0) {
		nls_error(NLS_ERRMSG_REDUCTION_TOO_DEEP);
		return NLS_EINFREDUCE;
	}
	switch ((*tree)->nn_type) {
	case NLS_TYPE_INT:
	case NLS_TYPE_FUNCTION:
		return 0;
	case NLS_TYPE_APPLICATION:
		if ((ret = nls_apply(tree, limit))) {
			return ret;
		}
		return nls_reduce(tree, limit);
	case NLS_TYPE_LIST:
		return nls_list_reduce(tree, limit);
	default:
		NLS_BUG(NLS_BUGMSG_INVALID_NODE_TYPE);
		return EINVAL; /* must not happen */
	}
}

static int
nls_list_reduce(nls_node **list, int limit)
{
	int ret;
	nls_node **item, *tmp;

	nls_list_foreach(*list, &item, &tmp) {
		ret = nls_reduce(item, --limit);
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
nls_apply(nls_node **node, int limit)
{
	int ret;
	nls_node *tmp;

	nls_application *app = &((*node)->nn_app);
	nls_function func = app->na_func->nn_func;
	nls_node **arg = &app->na_arg;

	if ((ret = nls_reduce(arg, --limit))) {
		return ret;
	}
	if ((ret = (func)(*arg, &tmp))) {
		return ret;
	}
	nls_tree_free(*node);
	*node = tmp;
	return 0;
}

static void
nls_list_item_free(nls_node *node)
{
	nls_list *list = &(node->nn_list);

	nls_tree_free(list->nl_head);
	if (list->nl_rest) {
		nls_tree_free(list->nl_rest);
	}
}

static void
nls_tree_free(nls_node *tree)
{
	switch (tree->nn_type) {
	case NLS_TYPE_INT:
		break;
	case NLS_TYPE_FUNCTION:
		break;
	case NLS_TYPE_LIST:
		nls_list_item_free(tree);
		break;
	case NLS_TYPE_APPLICATION:
		{
			nls_application *app = &tree->nn_app;

			nls_tree_free(app->na_arg);
			nls_free(app->na_func);
		}
		break;
	default:
		NLS_BUG(NLS_BUGMSG_INVALID_NODE_TYPE);
		return;
	}
	nls_free(tree);
}

static int
nls_tree_print(FILE *out, nls_node *tree)
{
	int ret;

	switch (tree->nn_type) {
	case NLS_TYPE_INT:
		fprintf(out, "%d", tree->nn_int);
		return 0;
	case NLS_TYPE_FUNCTION:
		fprintf(out, "func:%p", tree->nn_func);
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
		NLS_BUG(NLS_BUGMSG_INVALID_NODE_TYPE);
		return EINVAL;
	}
}
