#ifndef _NAMELESS_H_
#define _NAMELESS_H_

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "nameless/types.h"
#include "nameless/function.h"

/**********
 * MACROS *
 **********/
/* Constant */
#define NLS_LIST_ARRAY_EXP  8
#define NLS_REDUCTION_LIMIT 10000

/* Error Code */
#define NLS_EINFREDUCE EMLINK

/* Error Message */
#define NLS_ERRMSG_INVALID_NODE_TYPE  "Invalid node type."
#define NLS_ERRMSG_REDUCTION_TOO_DEEP "Reduction too deep."

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
#define nls_error(fmt, ...) \
	fprintf(nls_err, "ERROR:%s:%d:" fmt "\n", \
	__FILE__, __LINE__, ## __VA_ARGS__)

/* Memory Management */
#define nls_free(ptr) free(ptr)
#define nls_new(type) ((type*)malloc(sizeof(type)))
#define nls_array_new(type, n) ((type*)malloc(sizeof(type) * n))

/* List Management */
#define nls_list_foreach(tmp, list_node) \
	for (tmp = ((list_node)->nn_list.nl_array); \
		(tmp != (((list_node)->nn_list.nl_array) \
		 + ((list_node)->nn_list.nl_num))); \
		tmp++) \

/********************
 * GLOBAL VARIABLES *
 ********************/
extern FILE *yyin;
extern FILE *yyout;
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

/* Memory Management */
void nls_list_free(nls_node *list_node);
void nls_tree_free(nls_node *tree);

#endif /* _NAMELESS_H_ */
