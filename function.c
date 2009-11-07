#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include "nameless.h"
#include "nameless/node.h"
#include "nameless/mm.h"
#include "nameless/function.h"

#define NLS_MSG_TOO_MANY_ARGS "Too many arguments"

static int nls_op_add(int a, int b);
static int nls_op_sub(int a, int b);
static int nls_op_mul(int a, int b);
static int nls_op_div(int a, int b);
static int nls_op_mod(int a, int b);
static int _nls_int2_func(nls_fp fp, char *name, nls_int2_op op, nls_node *args, nls_node **out);
static int __nls_int2_func(nls_node *arg1, nls_node *arg2, nls_int2_op op, nls_node **out);
static int nls_argn_get(nls_node *args, int n, ...);
static int nls_curry_new(nls_fp fp, char *name, int num_lack, nls_node *args, nls_node **out);
static nls_node* nls_vars_new(int n);

NLS_DEF_INT2_FUNC(add);
static int
nls_op_add(int a, int b)
{
	return a + b;
}

NLS_DEF_INT2_FUNC(sub);
static int
nls_op_sub(int a, int b)
{
	return a - b;
}

NLS_DEF_INT2_FUNC(mul);
static int
nls_op_mul(int a, int b)
{
	return a * b;
}

NLS_DEF_INT2_FUNC(div);
static int
nls_op_div(int a, int b)
{
	return a / b;
}

NLS_DEF_INT2_FUNC(mod);
static int
nls_op_mod(int a, int b)
{
	return a % b;
}

int
nls_func_abst(nls_node *arg, nls_node **out)
{
	int ret;
	nls_node *node, **abst_vars, **abst_def;

	if ((ret = nls_argn_get(arg, 2, &abst_vars, &abst_def))) {
		return ret;
	}
	node = nls_abstraction_new(*abst_vars, *abst_def);
	if (!node) {
		return ENOMEM;
	}
	*out = node;
	return 0;
}

static int
_nls_int2_func(nls_fp fp, char *name, nls_int2_op op, nls_node *args, nls_node **out)
{
	int ret;
	nls_node **arg1, **arg2;

	switch (nls_list_count(args)) {
	case 1:
		return nls_curry_new(fp, name, 1, args, out);
	case 2:
		break;
	default:
		return EINVAL;
	}
	if ((ret = nls_argn_get(args, 2, &arg1, &arg2))) {
		return ret;
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

static int
nls_argn_get(nls_node *args, int n, ...)
{
	int i;
	nls_node **item, *tmp;
	nls_node ***out;
	va_list alist;

	va_start(alist, n);
	i = 0;
	nls_list_foreach(args, &item, &tmp) {
		if (++i > n) {
			break;
		}
		out = va_arg(alist, nls_node***);
		*out = item;
	}
	va_end(alist);
	return 0;
}

static int
nls_curry_new(nls_fp fp, char *name, int num_lack, nls_node *args, nls_node **out)
{
	nls_node *vars, *add_vars;
	nls_node *def, *func, *curry;

	vars = nls_vars_new(num_lack);
	if (!vars) {
		return ENOMEM;
	}
	add_vars = nls_vars_new(num_lack);
	if (!add_vars) {
		nls_node_release(nls_node_grab(vars));
		return ENOMEM;
	}
	func = nls_function_new(fp, name);
	if (!func) {
		nls_node_release(nls_node_grab(vars));
		nls_node_release(nls_node_grab(add_vars));
		return ENOMEM;
	}
	nls_list_concat(args, add_vars);
	def = nls_application_new(func, args);
	if (!def) {
		nls_node_release(nls_node_grab(func));
		nls_node_release(vars);
		return ENOMEM;
	}
	curry = nls_abstraction_new(vars, def);
	if (!curry) {
		nls_node_release(nls_node_grab(def));
		nls_node_release(vars);
		return ENOMEM;
	}
	*out = curry;
	return 0;
}

#define NLS_ANON_VAR_NAME_BUF_SIZE 32
#define NLS_ANON_VAR_PREFIX 'x'
static nls_node*
nls_vars_new(int n)
{
	int i;
	char *bufp, buf[NLS_ANON_VAR_NAME_BUF_SIZE];
	nls_node *var;
	nls_node *vars = NULL;

	buf[0] = NLS_ANON_VAR_PREFIX;
	bufp = buf + 1;
	for (i = 1; i <= n; i++) {
		nls_string *name;

		snprintf(bufp, NLS_ANON_VAR_NAME_BUF_SIZE-1, "%d", i);
		name = nls_string_new(buf);
		if (!name) {
			goto free_exit;
		}
		var = nls_var_new(name);
		if (!var) {
			nls_string_free(name);
			goto free_exit;
		}
		if (!vars) {
			vars = nls_list_new(var);
			if (!vars) {
				nls_node_release(nls_node_grab(var));
				return NULL;
			}
			continue;
		}
		nls_list_add(vars, var);
	}
	return vars;
free_exit:
	if (vars) {
		nls_node_release(nls_node_grab(vars));
	}
	return NULL;
}
