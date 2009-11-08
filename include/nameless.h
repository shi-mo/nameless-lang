#ifndef _NAMELESS_H_
#define _NAMELESS_H_

#include <stdio.h>
#include "nameless/node.h"
#include "nameless/hash.h"

#define NLS_GLOBAL /* empty */

#define NLS_MSG_INVALID_NODE_TYPE "Invalid node type"
#define NLS_MSG_INVALID_REFCOUNT  "Invalid reference count"
#define NLS_MSG_NOT_IMPLEMENTED   "Not implemented yet"
#define NLS_MSG_TOO_MANY_ARGS     "Too many arguments"
#define NLS_MSG_FAILED_TO_ALLOCATE_MEMORY "Failed to allocate memory"

#define NLS_WARN(fmt, ...) \
	fprintf(nls_sys_err, "Warning:%s:%d:%s: " fmt "\n", \
		__FILE__, __LINE__, __FUNCTION__, ## __VA_ARGS__)

#define NLS_ERROR(fmt, ...) \
	do { \
		fprintf(nls_sys_err, "ERROR:%s:%d:%s: " fmt "\n", \
			__FILE__, __LINE__, __FUNCTION__, ## __VA_ARGS__); \
		exit(1); \
	} while(0)

#define NLS_BUG(fmt, ...) \
	do { \
		fprintf(nls_sys_err, "BUG:%s:%d:%s: " fmt "\n", \
			__FILE__, __LINE__, __FUNCTION__, ## __VA_ARGS__); \
		exit(1); \
	} while(0)

#ifdef NLS_UNIT_TEST
# define NLS_ASSERT(expr) \
	do { \
		if (!(expr)) { \
			fprintf(stderr, "Assertion failure:%s:%d:%s\n", \
				__FILE__, __LINE__, __FUNCTION__); \
			exit(1); \
		} \
		fprintf(stdout, "."); \
	} while (0)

# define NLS_ASSERT_NOT(expr) \
	NLS_ASSERT(!(expr))

# define NLS_ASSERT_EQUALS(expected, actual) \
	NLS_ASSERT((expected) == (actual))

# define NLS_ASSERT_NOT_EQUALS(expected, actual) \
	NLS_ASSERT((expected) != (actual))
#endif /* NLS_UNIT_TEST */

extern FILE *nls_sys_out;
extern FILE *nls_sys_err;
extern nls_hash nls_sys_sym_table;

int nls_main(FILE *in, FILE *out, FILE *err);
void nls_init(FILE *out, FILE *err);
void nls_term(void);
int nls_reduce(nls_node **tree);
nls_node* nls_symbol_get(nls_string *name);

#endif /* _NAMELESS_H_ */
