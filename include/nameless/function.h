#ifndef _NAMELESS_FUNCTION_H_
#define _NAMELESS_FUNCTION_H_

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
