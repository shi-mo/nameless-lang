#ifndef _NAMELESS_H_
#define _NAMELESS_H_

#include <stdio.h>
#include "nameless/node.h"

#define NLS_GLOBAL /* empty */

#define NLS_MSG_INVALID_NODE_TYPE "Invalid node type"
#define NLS_MSG_INVALID_REFCOUNT  "Invalid reference count"
#define NLS_MSG_NOT_IMPLEMENTED   "Not implemented yet"

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

#ifdef NLS_UNIT_TEST
# define NLS_ASSERT_EQUALS(expected, actual) \
	do { \
		if ((expected) != (actual)) { \
			fprintf(stderr, "Assertion failure:%s:%d:%s\n", \
				__FILE__, __LINE__, __FUNCTION__); \
			exit(1); \
		} \
		fprintf(stdout, "."); \
	} while (0)

# define NLS_ASSERT_NOT_EQUALS(expr1, expr2) \
	do { \
		if ((expr1) == (expr2)) { \
			fprintf(stderr, "Assertion failure:%s:%d:%s\n", \
				__FILE__, __LINE__, __FUNCTION__); \
			exit(1); \
		} \
		fprintf(stdout, "."); \
	} while (0)
#endif /* NLS_UNIT_TEST */

/**
 * Traverse all items in nls_list.
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

extern FILE *nls_sys_out;
extern FILE *nls_sys_err;

int nls_main(FILE *in, FILE *out, FILE *err);
int nls_reduce(nls_node **tree);
nls_string* nls_string_new(char *s);
nls_string* nls_string_grab(nls_string *str);
void nls_string_release(nls_string *str);
void nls_string_free(nls_string *str);

#endif /* _NAMELESS_H_ */
