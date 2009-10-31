#ifndef _NAMELESS_NODE_H_
#define _NAMELESS_NODE_H_

typedef enum _nls_node_type {
	NLS_TYPE_INT,
	NLS_TYPE_VAR,
	NLS_TYPE_FUNCTION,
	NLS_TYPE_APPLICATION,
	NLS_TYPE_LIST,
} nls_node_type;

typedef struct _nls_string {
	int ns_len;
	char *ns_bufp;
} nls_string;

typedef struct _nls_var {
	nls_string *nv_name;
} nls_var;

struct _nls_node;
typedef struct _nls_application {
	struct _nls_node *na_func;
	struct _nls_node *na_arg;
} nls_application;

typedef struct _nls_list {
	struct _nls_node *nl_head;
	struct _nls_node *nl_rest;
} nls_list;

typedef int (*nls_function)(struct _nls_node*, struct _nls_node**);

typedef struct _nls_node {
	nls_node_type nn_type;
	union {
		int nnu_int;
		nls_var nnu_var;
		nls_list nnu_list;
		nls_function nnu_func;
		nls_application nnu_app;
	} nn_u;
} nls_node;

#define nn_int  nn_u.nnu_int
#define nn_var  nn_u.nnu_var
#define nn_list nn_u.nnu_list
#define nn_func nn_u.nnu_func
#define nn_app  nn_u.nnu_app

#endif /* _NAMELESS_NODE_H_ */
