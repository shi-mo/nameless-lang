#include "y.tab.h"
#include "binop.h"

binop_node_list *binop_parse_result;

static int binop_list_eval(binop_node_list *list);
static int binop_eval(binop_node *tree);
static int binop_apply(binop_operator, int, int);

static void binop_list_free(binop_node_list *list);
static void binop_tree_free(binop_node *tree);

int
main(int argc, char *argv[])
{
	int ret;
	binop_node_list *list = NULL;

	binop_out = stdout;

	ret = yyparse();
	if (!ret && binop_parse_result) {
		list = binop_parse_result;
		ret = binop_list_eval(list);
	}
	if (list) {
		binop_list_free(list);
	}

	return ret;
}

static int
binop_list_eval(binop_node_list *list)
{
	int ret;
	binop_node_list *tmp;

	binop_list_foreach(tmp, list) {
		ret = binop_eval(tmp->bnl_node);
		BINOP_CONSOLE("%d", ret);
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
binop_eval(binop_node *tree)
{
	switch (tree->bn_type) {
	case BINOP_TYPE_INT:
		return tree->bn_union.bnu_int;
	case BINOP_TYPE_OPERATOR:
		return 0;
	case BINOP_TYPE_APPLICATION:
		{
			binop_application *app = &(tree->bn_union.bnu_app);

			return binop_apply(app->ba_op->bn_union.bnu_op,
				binop_eval(app->ba_left),
				binop_eval(app->ba_right));
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
binop_apply(binop_operator op, int a, int b)
{
	return (op)(a, b);
}

static void
binop_list_free(binop_node_list *list)
{
	binop_node_list *tmp;

	binop_list_foreach(tmp, list) {
		binop_tree_free(tmp->bnl_node);
		binop_free(tmp);
	}
}

static void
binop_tree_free(binop_node *tree)
{
	if (BINOP_TYPE_APPLICATION == tree->bn_type) {
		binop_application *app = &tree->bn_union.bnu_app;

		binop_tree_free(app->ba_left);
		binop_tree_free(app->ba_right);
	}
	binop_free(tree);
}
