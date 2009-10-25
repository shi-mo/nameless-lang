#include "y.tab.h"
#include "nameless.h"

nls_node *nls_parse_result;

static int nls_eval(nls_node *tree);
static int nls_apply(nls_function, int, int);

static void nls_list_free(nls_node *list_node);
static void nls_tree_free(nls_node *tree);

int
main(int argc, char *argv[])
{
	int ret;
	nls_node *tree = NULL;

	nls_out = stdout;
	nls_err = stderr;

	ret = yyparse();
	if (!ret && nls_parse_result) {
		tree = nls_parse_result;
		ret = nls_eval(tree);
	}
	if (tree) {
		nls_list_free(tree);
	}

	return ret;
}

/*
 * @return positive or 0: evaluation result
 * 	   negative	: error code
 */
static int
nls_eval(nls_node *tree)
{
	switch (tree->nn_type) {
	case NLS_TYPE_INT:
		return tree->nn_union.nnu_int;
	case NLS_TYPE_OPERATOR:
		return 0;
	case NLS_TYPE_APPLICATION:
		{
			nls_application *app = &(tree->nn_union.nnu_app);

			return nls_apply(app->na_op->nn_union.nnu_op,
				nls_eval(app->na_left),
				nls_eval(app->na_right));
		}
	case NLS_TYPE_LIST:
		{
			int ret;
			nls_node **tmp;

			nls_list_foreach(tmp, tree) {
				ret = nls_eval(*tmp);
				NLS_CONSOLE("%d", ret);
				if (ret < 0) {
					return -ret;
				}
			}
			return 0;
		}
	default:
		NLS_ERRMSG("Illegal node type.");
		return -1; /* must not happen */
	}
}

/*
 * TODO: Reconsider value returning (negative expr).
 * @return positive or 0: evaluation result
 * 	   negative	: error code
 */
static int
nls_apply(nls_function op, int a, int b)
{
	return (op)(a, b);
}

static void
nls_list_free(nls_node *list_node)
{
	nls_node **tmp;

	nls_list_foreach(tmp, list_node) {
		nls_tree_free(*tmp);
	}
	nls_free(list_node->nn_union.nnu_list.nl_array);
	nls_free(list_node);
}

static void
nls_tree_free(nls_node *tree)
{
	if (NLS_TYPE_LIST == tree->nn_type) {
		nls_list_free(tree);
		return;
	}
	if (NLS_TYPE_APPLICATION == tree->nn_type) {
		nls_application *app = &tree->nn_union.nnu_app;

		nls_tree_free(app->na_left);
		nls_tree_free(app->na_right);
	}
	nls_free(tree);
}
