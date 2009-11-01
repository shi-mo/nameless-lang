#include <stdio.h>
#include <errno.h>
#include "y.tab.h"
#include "nameless.h"
#include "nameless/parser.h"
#include "nameless/mm.h"

#define NLS_MSG_REDUCTION_TOO_DEEP "Reduction too deep"
#define NLS_MSG_INVALID_NODE_TYPE  "Invalid node type"

NLS_GLOBAL FILE *nls_sys_out;
NLS_GLOBAL FILE *nls_sys_err;

static int nls_list_reduce(nls_node **tree);
static int nls_apply(nls_node **node);
static void nls_list_item_free(nls_node *list);
static void nls_tree_free(nls_node *tree);
static void nls_string_free(nls_string *str);
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
	tree = nls_sys_parse_result;
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

/*
 * Recursively do application
 */
int
nls_reduce(nls_node **tree)
{
	int ret;

	switch ((*tree)->nn_type) {
	case NLS_TYPE_INT:
	case NLS_TYPE_VAR:
	case NLS_TYPE_FUNCTION:
		return 0;
	case NLS_TYPE_APPLICATION:
		if ((ret = nls_apply(tree))) {
			return ret;
		}
		return nls_reduce(tree);
	case NLS_TYPE_LIST:
		return nls_list_reduce(tree);
	default:
		NLS_BUG(NLS_MSG_INVALID_NODE_TYPE);
		return EINVAL; /* must not happen */
	}
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
	nls_node *tmp;

	nls_application *app = &((*node)->nn_app);
	nls_function func = app->na_func->nn_func;
	nls_node **arg = &app->na_arg;

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
	case NLS_TYPE_VAR:
		nls_string_free(tree->nn_var.nv_name);
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
		NLS_BUG(NLS_MSG_INVALID_NODE_TYPE);
		return;
	}
	nls_free(tree);
}

static void
nls_string_free(nls_string *str)
{
	nls_free(str->ns_bufp);
	nls_free(str);
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
