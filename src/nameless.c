#include "y.tab.h"
#include "nameless.h"

nls_node *nls_parse_result;

static int nls_eval(nls_node *tree);
static int nls_apply(nls_node **node);

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
 * @return  0: success
 * 	   !0: error code
 */
static int
nls_eval(nls_node *tree)
{
	switch (tree->nn_type) {
	case NLS_TYPE_INT:
		return 0;
	case NLS_TYPE_OPERATOR:
		return 0;
	case NLS_TYPE_APPLICATION:
		{
			nls_application *app = &(tree->nn_app);
			nls_eval(app->na_arg);

			return nls_apply(&tree);
		}
	case NLS_TYPE_LIST:
		{
			int ret;
			nls_node **tmp;

			nls_list_foreach(tmp, tree) {
				ret = nls_eval(*tmp);
				if (ret) {
					return ret;
				}
			}
			return 0;
		}
	default:
		NLS_ERRMSG("Illegal node type.");
		return 1; /* must not happen */
	}
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
	nls_node *arg = app->na_arg;

	if ((ret = (func)(arg, &tmp))) {
		nls_tree_free(*node);
		*node = tmp;
	}
	return ret;
}

void
nls_list_free(nls_node *list_node)
{
	nls_node **tmp;

	nls_list_foreach(tmp, list_node) {
		nls_tree_free(*tmp);
	}
	nls_free(list_node->nn_list.nl_array);
	nls_free(list_node);
}

void
nls_tree_free(nls_node *tree)
{
	if (NLS_TYPE_LIST == tree->nn_type) {
		nls_list_free(tree);
		return;
	}
	if (NLS_TYPE_APPLICATION == tree->nn_type) {
		nls_application *app = &tree->nn_app;

		nls_tree_free(app->na_arg);
	}
	nls_free(tree);
}
