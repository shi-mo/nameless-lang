#ifndef _NLS_H_
#define _NLS_H_

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

/**********
 * MACROS *
 **********/
/* Debug */
#ifdef NLS_DEBUG
# define NLS_DEBUG_PRINT(fmt, ...) \
	fprintf(stderr, "Debug:%s:%d:" fmt "\n", \
	__FILE__, __LINE__, ## __VA_ARGS__)
#else  /* NLS_DEBUG */
# define NLS_DEBUG_PRINT(fmt, ...) \
	do {} while(0)
#endif /* NLS_DEBUG */

/* Output */
#define NLS_CONSOLE(fmt, ...) \
	fprintf(nls_out, "> " fmt "\n", ## __VA_ARGS__)

/* Memory Management */
#define nls_free(ptr) free(ptr)
#define nls_new(type) ((type*)malloc(sizeof(type)))

#define NLS_OPERATOR_NEW(fp) \
	({ \
		nls_node *_nd = nls_new(nls_node); \
		!_nd ? NULL : ({ \
			_nd->nn_type = NLS_TYPE_OPERATOR; \
			_nd->nn_union.nnu_op = (fp); \
			_nd; \
		}); \
	})

#define NLS_INT_NEW(val) \
	({ \
		nls_node *_nd = nls_new(nls_node); \
		!_nd ? NULL : ({ \
			_nd->nn_type = NLS_TYPE_INT; \
			_nd->nn_union.nnu_int = (val); \
			_nd; \
		}); \
	})

#define NLS_APPLICATION_NEW(op, left, right) \
	({ \
		nls_node *_nd = nls_new(nls_node); \
		!_nd ? NULL : ({ \
			nls_application *_app; \
			_nd->nn_type = NLS_TYPE_APPLICATION; \
			_app = &(_nd->nn_union.nnu_app); \
			_app->na_op    = (op); \
			_app->na_left  = (left); \
			_app->na_right = (right); \
			_nd; \
		}); \
	})

/* List Management */
#define NLS_NODE_LIST_NEW(nd) \
	({ \
		nls_node_list *_lst = nls_new(nls_node_list); \
		!_lst ? NULL : ({ \
			_lst->nnl_node = (nd); \
			_lst->nnl_prev = _lst; \
			_lst->nnl_next = NULL; \
			_lst; \
		}); \
	})

#define NLS_NODE_LIST_ADD(lst, nd) \
	({ \
		nls_node_list *_new_lst = NLS_NODE_LIST_NEW(nd); \
		if (_new_lst) { \
			(lst)->nnl_prev->nnl_next = _new_lst; \
			_new_lst->nnl_prev = (lst)->nnl_prev; \
			(lst)->nnl_prev = _new_lst; \
		} \
		(lst); \
	})

#define nls_list_foreach(tmp, list) \
	for(tmp = (list); tmp; tmp = tmp->nnl_next)

/*********
 * TYPES *
 *********/
typedef enum _nls_node_type {
	NLS_TYPE_INT,
	NLS_TYPE_OPERATOR,
	NLS_TYPE_APPLICATION,
} nls_node_type;

typedef int (*nls_operator)(int, int);

struct _nls_node;
typedef struct _nls_application {
	struct _nls_node *na_op;
	struct _nls_node *na_left;
	struct _nls_node *na_right;
} nls_application;

typedef struct _nls_node {
	nls_node_type nn_type;
	union {
		int nnu_int;
		nls_operator nnu_op;
		nls_application nnu_app;
	} nn_union;
} nls_node;

typedef struct _nls_node_list {
	nls_node *nnl_node;
	struct _nls_node_list *nnl_next;
	struct _nls_node_list *nnl_prev;
} nls_node_list;

/********************
 * GLOBAL VARIABLES *
 ********************/
extern FILE *nls_out;
extern nls_node_list *nls_parse_result;

/**************
 * PROTOTYPES *
 **************/
int yylex(void);
int yyparse(void);
int yyerror(char *msg);

#endif /* _NLS_H_ */
