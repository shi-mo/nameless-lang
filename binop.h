#ifndef _BINOP_H_
#define _BINOP_H_

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

/**********
 * MACROS *
 **********/
/* Debug */
#ifdef BINOP_DEBUG
# define BINOP_DEBUG_PRINT(fmt, ...) \
	fprintf(stderr, "Debug:%s:%d:" fmt "\n", \
	__FILE__, __LINE__, ## __VA_ARGS__)
#else  /* BINOP_DEBUG */
# define BINOP_DEBUG_PRINT(fmt, ...) \
	do {} while(0)
#endif /* BINOP_DEBUG */

/* Output */
#define BINOP_CONSOLE(fmt, ...) \
	fprintf(binop_out, "> " fmt "\n", ## __VA_ARGS__)

/* Memory Management */
#define binop_free(ptr) free(ptr)
#define binop_new(type) ((type*)malloc(sizeof(type)))

#define BINOP_OPERATOR_NEW(fp) \
	({ \
		binop_node *_nd = binop_new(binop_node); \
		!_nd ? NULL : ({ \
			_nd->bn_type = BINOP_TYPE_OPERATOR; \
			_nd->bn_union.bnu_op = (fp); \
			_nd; \
		}); \
	})

#define BINOP_INT_NEW(val) \
	({ \
		binop_node *_nd = binop_new(binop_node); \
		!_nd ? NULL : ({ \
			_nd->bn_type = BINOP_TYPE_INT; \
			_nd->bn_union.bnu_int = (val); \
			_nd; \
		}); \
	})

#define BINOP_APPLICATION_NEW(op, left, right) \
	({ \
		binop_node *_nd = binop_new(binop_node); \
		!_nd ? NULL : ({ \
			binop_application *_app; \
			_nd->bn_type = BINOP_TYPE_APPLICATION; \
			_app = &(_nd->bn_union.bnu_app); \
			_app->ba_op    = (op); \
			_app->ba_left  = (left); \
			_app->ba_right = (right); \
			_nd; \
		}); \
	})

/* List Management */
#define BINOP_NODE_LIST_NEW(nd) \
	({ \
		binop_node_list *_lst = binop_new(binop_node_list); \
		!_lst ? NULL : ({ \
			_lst->bnl_node = (nd); \
			_lst->bnl_prev = _lst; \
			_lst->bnl_next = NULL; \
			_lst; \
		}); \
	})

#define BINOP_NODE_LIST_ADD(lst, nd) \
	({ \
		binop_node_list *_new_lst = BINOP_NODE_LIST_NEW(nd); \
		if (_new_lst) { \
			(lst)->bnl_prev->bnl_next = _new_lst; \
			_new_lst->bnl_prev = (lst)->bnl_prev; \
			(lst)->bnl_prev = _new_lst; \
		} \
		(lst); \
	})

#define binop_list_foreach(tmp, list) \
	for(tmp = (list); tmp; tmp = tmp->bnl_next)

/*********
 * TYPES *
 *********/
typedef enum _binop_node_type {
	BINOP_TYPE_INT,
	BINOP_TYPE_OPERATOR,
	BINOP_TYPE_APPLICATION,
} binop_node_type;

typedef int (*binop_operator)(int, int);

struct _binop_node;
typedef struct _binop_application {
	struct _binop_node *ba_op;
	struct _binop_node *ba_left;
	struct _binop_node *ba_right;
} binop_application;

typedef struct _binop_node {
	binop_node_type bn_type;
	union {
		int bnu_int;
		binop_operator bnu_op;
		binop_application bnu_app;
	} bn_union;
} binop_node;

typedef struct _binop_node_list {
	binop_node *bnl_node;
	struct _binop_node_list *bnl_next;
	struct _binop_node_list *bnl_prev;
} binop_node_list;

/********************
 * GLOBAL VARIABLES *
 ********************/
extern FILE *binop_out;
extern binop_node_list *binop_parse_result;

/**************
 * PROTOTYPES *
 **************/
int yylex(void);
int yyparse(void);
int yyerror(char *msg);

#endif /* _BINOP_H_ */
