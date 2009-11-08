#ifndef _NAMELESS_FUNCTION_H_
#define _NAMELESS_FUNCTION_H_

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

#include "nameless/node.h"

#define NLS_DEF_INT2_FUNC(name) \
	int \
	nls_func_##name(nls_node *args, nls_node **out) \
	{ \
		return _nls_int2_func(nls_func_##name, #name, \
			nls_op_##name, args, out); \
	}

typedef int (*nls_int2_op)(int a, int b);

int nls_func_add(nls_node*, nls_node**);
int nls_func_sub(nls_node*, nls_node**);
int nls_func_mul(nls_node*, nls_node**);
int nls_func_div(nls_node*, nls_node**);
int nls_func_mod(nls_node*, nls_node**);
int nls_func_abst(nls_node*, nls_node**);

#endif /* _NAMELESS_FUNCTION_H_ */
