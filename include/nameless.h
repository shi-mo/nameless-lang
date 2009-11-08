#ifndef _NAMELESS_H_
#define _NAMELESS_H_

/*
 * Nameless - A lambda calculation language.
 * Copyright (C) 2009 Yoshifumi Shimono <yoshifumi.shimono@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
