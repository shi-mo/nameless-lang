#include <errno.h>
#include "nameless.h"
#include "nameless/node.h"
#include "nameless/mm.h"
#include "nameless/function.h"

#define NLS_ISINT(node) (NLS_TYPE_INT == (node)->nn_type)
#define NLS_INT_VAL(node) ((node)->nn_int)
#define NLS_MSG_TOO_MANY_ARGS "Too many arguments"

static int nls_op_add(int a, int b);
static int nls_op_sub(int a, int b);
static int nls_op_mul(int a, int b);
static int nls_op_div(int a, int b);
static int nls_op_mod(int a, int b);
static int _nls_int2_func(nls_node *args, nls_int2_op op, nls_node **out);
static int __nls_int2_func(nls_node *arg1, nls_node *arg2, nls_int2_op op, nls_node **out);

NLS_DEF_INT2_FUNC(nls_func_add, nls_op_add);
static int
nls_op_add(int a, int b)
{
	return a + b;
}

NLS_DEF_INT2_FUNC(nls_func_sub, nls_op_sub);
static int
nls_op_sub(int a, int b)
{
	return a - b;
}

NLS_DEF_INT2_FUNC(nls_func_mul, nls_op_mul);
static int
nls_op_mul(int a, int b)
{
	return a * b;
}

NLS_DEF_INT2_FUNC(nls_func_div, nls_op_div);
static int
nls_op_div(int a, int b)
{
	return a / b;
}

NLS_DEF_INT2_FUNC(nls_func_mod, nls_op_mod);
static int
nls_op_mod(int a, int b)
{
	return a % b;
}

static int
_nls_int2_func(nls_node *args, nls_int2_op op, nls_node **out)
{
	int i;
	int ret;
	nls_node **arg1, **arg2;
	nls_node **item, *tmp;

	i = 0;
	nls_list_foreach(args, &item, &tmp) {
		switch (++i) {
		case 1:
			arg1 = item;
			break;
		case 2:
			arg2 = item;
			break;
		default:
			goto err_exit;
		}
	}
	if ((ret = nls_reduce(arg1))) {
		return ret;
	}
	if ((ret = nls_reduce(arg2))) {
		return ret;
	}
	if ((ret = __nls_int2_func(*arg1, *arg2, op, out))) {
		return ret;
	}
	return 0;
err_exit:
	NLS_ERROR(NLS_MSG_TOO_MANY_ARGS);
	return EINVAL;
}

static int
__nls_int2_func(nls_node *arg1, nls_node *arg2, nls_int2_op op, nls_node **out)
{
	nls_node *node;
	int result;

	if (!NLS_ISINT(arg1) || !NLS_ISINT(arg2)) {
		return EINVAL;
	}
	result = (op)(NLS_INT_VAL(arg1), NLS_INT_VAL(arg2));
	node = nls_int_new(result);
	if (!node) {
		return ENOMEM;
	}
	*out = node;
	return 0;
}

int
nls_func_abst(nls_node *arg, nls_node **out)
{
	int i;
	nls_node **item, *tmp;
	nls_node *node, *abst_vars, **abst_def;

	i = 0;
	nls_list_foreach(arg, &item, &tmp) {
		switch (++i) {
		case 1:
			abst_vars = *item;
			break;
		case 2:
			abst_def = item;
			break;
		default:
			goto new_abst;
		}
	}
new_abst:
	node = nls_abstraction_new(abst_vars, abst_def);
	if (!node) {
		return 1;
	}
	*out = node;
	return 0;
}
