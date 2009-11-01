#ifndef _NAMELESS_H_
#define _NAMELESS_H_

#include <stdio.h>
#include "nameless/node.h"

/**********
 * MACROS *
 **********/
#define NLS_GLOBAL /* empty */

/* Output */
#define NLS_WARN(fmt, ...) \
	fprintf(nls_sys_err, "Warning:%s:%d: " fmt "\n", \
		__FILE__, __LINE__, ## __VA_ARGS__)

#define NLS_ERROR(fmt, ...) \
	do { \
		fprintf(nls_sys_err, "ERROR:%s:%d: " fmt "\n", \
			__FILE__, __LINE__, ## __VA_ARGS__); \
		exit(1); \
	} while(0)

#define NLS_BUG(fmt, ...) \
	do { \
		fprintf(nls_sys_err, "BUG:%s:%d: " fmt "\n", \
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
int nls_reduce(nls_node **tree);
nls_node* nls_int_new(int val);

#endif /* _NAMELESS_H_ */
