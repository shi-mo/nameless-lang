/*
 * Nameless - A lambda calculation language.
 * Copyright (C) 2009 Yoshifumi Shimono <yoshifumi.shimono@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include "nameless.h"
#include "nameless/node.h"
#include "nameless/mm.h"
#include "nameless/function.h"

static int nls_op_add(int a, int b);
static int nls_op_sub(int a, int b);
static int nls_op_mul(int a, int b);
static int nls_op_div(int a, int b);
static int nls_op_mod(int a, int b);
static int _nls_int2_func(nls_fp fp, char *name, nls_int2_op op, nls_node *args, nls_node **out);
static int __nls_int2_func(nls_node *arg1, nls_node *arg2, nls_int2_op op, nls_node **out);
static int nls_argn_get(nls_node *args, int n, ...);

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

int
nls_func_set(nls_node *arg, nls_node**out)
{
	int ret;
	nls_node **var, **def;

	if ((ret = nls_argn_get(arg, 2, &var, &def))) {
		return ret;
	}
	if (!NLS_ISVAR(*var)) {
		return EINVAL;
	}
	ret = nls_hash_add(&nls_sys_sym_table, (*var)->nn_var.nv_name, *def);
	if (ret) {
		return ret;
	}
	nls_release(var);
	*out = *def;
	return 0;
}

static int
_nls_int2_func(nls_fp fp, char *name, nls_int2_op op, nls_node *args, nls_node **out)
{
	int ret;
	nls_node **arg1, **arg2;

	if ((ret = nls_argn_get(args, 2, &arg1, &arg2))) {
		return ret;
	}
	if ((ret = nls_apply(arg1))) {
		return ret;
	}
	if ((ret = nls_apply(arg2))) {
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
