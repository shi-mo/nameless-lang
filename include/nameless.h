#ifndef _NLS_H_
#define _NLS_H_

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include "nameless/function.h"

/**********
 * MACROS *
 **********/
/* Constant */
#define NLS_LIST_ARRAY_EXP 8

/* Debug */
#ifdef NLS_DEBUG
# define NLS_DEBUG_PRINT(fmt, ...) \
	fprintf(stderr, "DEBUG:%s:%d:" fmt "\n", \
	__FILE__, __LINE__, ## __VA_ARGS__)
#else  /* NLS_DEBUG */
# define NLS_DEBUG_PRINT(fmt, ...) \
	do {} while(0)
#endif /* NLS_DEBUG */

/* Output */
#define NLS_CONSOLE(fmt, ...) \
	fprintf(nls_out, "> " fmt "\n", ## __VA_ARGS__)

#define NLS_ERRMSG(fmt, ...) \
	fprintf(nls_err, "ERROR:%s:%d:" fmt "\n", \
	__FILE__, __LINE__, ## __VA_ARGS__)

/* Memory Management */
#define nls_free(ptr) free(ptr)
#define nls_new(type) ((type*)malloc(sizeof(type)))
#define nls_array_new(type, n) ((type*)malloc(sizeof(type) * n))

/* List Management */
#define nls_list_foreach(tmp, list_node) \
	for(tmp = ((list_node)->nn_union.nnu_list.nl_array); \
		(tmp != (((list_node)->nn_union.nnu_list.nl_array) \
		 + ((list_node)->nn_union.nnu_list.nl_num))); \
		tmp++) \

/*********
 * TYPES *
 *********/
typedef enum _nls_node_type {
	NLS_TYPE_INT,
	NLS_TYPE_OPERATOR,
	NLS_TYPE_APPLICATION,
	NLS_TYPE_LIST,
} nls_node_type;

typedef int (*nls_operator)(int, int);

struct _nls_node;
typedef struct _nls_application {
	struct _nls_node *na_op;
	struct _nls_node *na_left;
	struct _nls_node *na_right;
} nls_application;

typedef struct _nls_list {
	int nl_num;
	int nl_ary_size;
	struct _nls_node **nl_array;
} nls_list;

typedef struct _nls_node {
	nls_node_type nn_type;
	union {
		int nnu_int;
		nls_list nnu_list;
		nls_operator nnu_op;
		nls_application nnu_app;
	} nn_union;
} nls_node;

/********************
 * GLOBAL VARIABLES *
 ********************/
extern FILE *nls_out;
extern FILE *nls_err;
extern nls_node *nls_parse_result;

/**************
 * PROTOTYPES *
 **************/
/* Parser & Lexer */
int yylex(void);
int yyparse(void);
int yyerror(char *msg);

#endif /* _NLS_H_ */
