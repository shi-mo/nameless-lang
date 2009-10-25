#include "y.tab.h"
#include "nameless.h"

nls_node_list *nls_parse_result;

static int nls_list_eval(nls_node_list *list);
static int nls_eval(nls_node *tree);
static int nls_apply(nls_operator, int, int);

static void nls_list_free(nls_node_list *list);
static void nls_tree_free(nls_node *tree);

int
main(int argc, char *argv[])
{
	int ret;
	nls_node_list *list = NULL;

	nls_out = stdout;

	ret = yyparse();
	if (!ret && nls_parse_result) {
		list = nls_parse_result;
		ret = nls_list_eval(list);
	}
	if (list) {
		nls_list_free(list);
	}

	return ret;
}

static int
nls_list_eval(nls_node_list *list)
{
	int ret;
	nls_node_list *tmp;

	nls_list_foreach(tmp, list) {
		ret = nls_eval(tmp->nnl_node);
		NLS_CONSOLE("%d", ret);
		if (ret < 0) {
			return -ret;
		}
	}
	return 0;
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
	default:
		return -1; /* must not happen */
	}
}

/*
 * TODO: Reconsider value returning (negative expr).
 * @return positive or 0: evaluation result
 * 	   negative	: error code
 */
static int
nls_apply(nls_operator op, int a, int b)
{
	return (op)(a, b);
}

static void
nls_list_free(nls_node_list *list)
{
	nls_node_list *tmp;

	nls_list_foreach(tmp, list) {
		nls_tree_free(tmp->nnl_node);
		nls_free(tmp);
	}
}

static void
nls_tree_free(nls_node *tree)
{
	if (NLS_TYPE_APPLICATION == tree->nn_type) {
		nls_application *app = &tree->nn_union.nnu_app;

		nls_tree_free(app->na_left);
		nls_tree_free(app->na_right);
	}
	nls_free(tree);
}
