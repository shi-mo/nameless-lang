#ifndef _NAMELESS_H_
#define _NAMELESS_H_

#include <stdio.h>
#include "nameless/node.h"

/**********
 * MACROS *
 **********/
#define NLS_GLOBAL /* empty */

/* Constant */
#define NLS_REDUCTION_LIMIT 10000

/* Error Code */
#define NLS_EINFREDUCE EMLINK

/* Error Message */
#define NLS_ERRMSG_REDUCTION_TOO_DEEP "Reduction too deep."
#define NLS_ERRFMT_MEMLEAK_DETECTED \
	"Memory leak detected: mem=%p ptr=%p ref=%d size=%d"

/* BUG Message */
#define NLS_BUGMSG_INVALID_NODE_TYPE "Invalid node type."
#define NLS_BUGMSG_BROKEN_LIST       "Broken list."
#define NLS_BUGMSG_BROKEN_MEMCHAIN   "Broken memchain."
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
	fprintf(nls_sys_err, "ERROR:%s:%d:" fmt "\n", \
		__FILE__, __LINE__, ## __VA_ARGS__)

#define NLS_BUG(fmt, ...) \
	do { \
		fprintf(nls_sys_err, "BUG:%s:%d:" fmt "\n", \
			__FILE__, __LINE__, ## __VA_ARGS__); \
		exit(1); \
	} while(0)

/*
 * @see nls_list_reduce()
 */
#define nls_list_foreach(list, item, tmp) \
	for ( \
		(*(item)) = &((list)->nn_list.nl_head), \
			(*(tmp)) = (list)->nn_list.nl_rest; \
		(*(item)); \
		(*(item)) = ((*(tmp)) ? &((*(tmp))->nn_list.nl_head) : NULL), \
			(*(tmp)) = ((*(tmp)) ? \
				(*(tmp))->nn_list.nl_rest : NULL) \
	) \

/********************
 * GLOBAL VARIABLES *
 ********************/
extern FILE *nls_sys_out;
extern FILE *nls_sys_err;

/**************
 * PROTOTYPES *
 **************/
int nls_main(FILE *in, FILE *out, FILE *err);

#endif /* _NAMELESS_H_ */
