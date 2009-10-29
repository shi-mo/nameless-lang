#ifndef _NAMELESS_NODE_H_
#define _NAMELESS_NODE_H_

typedef enum _nls_node_type {
	NLS_TYPE_INT,
	NLS_TYPE_FUNCTION,
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

#define nn_int  nn_u.nnu_int
#define nn_list nn_u.nnu_list
#define nn_func nn_u.nnu_func
#define nn_app  nn_u.nnu_app

#endif /* _NAMELESS_NODE_H_ */
