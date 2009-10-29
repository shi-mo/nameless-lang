#ifndef _NAMELESS_H_
#define _NAMELESS_H_

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "nameless/node.h"
#include "nameless/mm.h"
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
#define NLS_ERRFMT_MEMLEAK_DETECTED \
	"Memory leak detected: mem=%p ptr=%p ref=%d size=%d"

/* BUG Message */
#define NLS_BUGMSG_BROKEN_MEMCHAIN "Broken memchain."
#define NLS_BUGMSG_ILLEGAL_MEMCHAIN_OPERATION "Illegal memchain operation."
#define NLS_BUGFMT_ILLEGAL_ALLOCNT \
	"Illegal alloc count: alloc=%d free=%d"
#define NLS_BUGFMT_INVALID_REFCOUNT \
	"Invalid reference count: mem=%p ptr=%p ref=%d size=%d"

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

#define nls_bug(fmt, ...) \
	do { \
		fprintf(nls_err, "BUG:%s:%d:" fmt "\n", \
		__FILE__, __LINE__, ## __VA_ARGS__); \
		exit(1); \
	} while(0)

/* List Management */
#define nls_list_foreach(tmp, list_node) \
	for ((tmp) = ((list_node)->nn_list.nl_array); \
		((tmp) != (((list_node)->nn_list.nl_array) \
		 + ((list_node)->nn_list.nl_num))); \
		(tmp)++) \

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
int nls_main(FILE *in, FILE *out, FILE *err);

#endif /* _NAMELESS_H_ */
