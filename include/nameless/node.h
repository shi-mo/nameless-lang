#ifndef _NAMELESS_NODE_H_
#define _NAMELESS_NODE_H_

typedef enum _nls_node_type {
	NLS_TYPE_INT = 1,
	NLS_TYPE_VAR,
	NLS_TYPE_FUNCTION,
	NLS_TYPE_ABSTRACTION,
	NLS_TYPE_APPLICATION,
	NLS_TYPE_LIST,
} nls_node_type;

typedef struct _nls_string {
	int ns_ref;
	int ns_len;
	int ns_hash;
	char *ns_bufp;
} nls_string;

typedef struct _nls_var {
	nls_string *nv_name;
} nls_var;

struct _nls_node;
typedef struct _nls_arg_ref {
	struct _nls_arg_ref *nar_next;
	struct _nls_node **nar_node;
} nls_arg_ref;

typedef struct _nls_abstraction {
	int nab_num_arg;
	nls_arg_ref **nab_arg_ref;
	struct _nls_node *nab_def;
} nls_abstraction;

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
	int nn_ref;
	nls_node_type nn_type;
	union {
		int nnu_int;
		nls_var nnu_var;
		nls_list nnu_list;
		nls_function nnu_func;
		nls_abstraction nnu_abst;
		nls_application nnu_app;
	} nn_u;
} nls_node;

#define nn_int  nn_u.nnu_int
#define nn_var  nn_u.nnu_var
#define nn_list nn_u.nnu_list
#define nn_func nn_u.nnu_func
#define nn_abst nn_u.nnu_abst
#define nn_app  nn_u.nnu_app

nls_node* nls_node_grab(nls_node *node);
void nls_node_release(nls_node *tree);
nls_node* nls_int_new(int val);
nls_node* nls_var_new(nls_string *name);
nls_node* nls_function_new(nls_function func);
nls_node* nls_abstraction_new(nls_node *vars, nls_node **def);
nls_node* nls_application_new(nls_node *func, nls_node *arg);
nls_node* nls_list_new(nls_node *node);
int nls_list_add(nls_node *list, nls_node *node);

#endif /* _NAMELESS_NODE_H_ */
