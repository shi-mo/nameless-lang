#ifndef _NLS_TYPES_H_
#define _NLS_TYPES_H_

typedef enum _nls_node_type {
	NLS_TYPE_INT,
	NLS_TYPE_OPERATOR,
	NLS_TYPE_APPLICATION,
	NLS_TYPE_LIST,
} nls_node_type;

struct _nls_node;
typedef struct _nls_application {
	struct _nls_node *na_func;
	struct _nls_node *na_arg;
} nls_application;

typedef struct _nls_list {
	int nl_num;
	int nl_ary_size;
	struct _nls_node **nl_array;
} nls_list;

typedef int (*nls_function)(struct _nls_node*, struct _nls_node**);

typedef struct _nls_node {
	nls_node_type nn_type;
	union {
		int nnu_int;
		nls_list nnu_list;
		nls_function nnu_func;
		nls_application nnu_app;
	} nn_u;
} nls_node;

#endif /* _NAMELESS_TYPES_H_ */
