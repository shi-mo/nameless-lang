#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "y.tab.h"
#include "nameless.h"
#include "nameless/parser.h"
#include "nameless/mm.h"

#define NLS_HASH_WIDTH 31

NLS_GLOBAL FILE *nls_sys_out;
NLS_GLOBAL FILE *nls_sys_err;

static size_t nls_strnlen_hash(char *str, size_t n, int *hashp);
static int nls_list_reduce(nls_node **tree);
static int nls_apply(nls_node **node);
static int nls_tree_print(FILE *out, nls_node *tree);

int
nls_main(FILE *in, FILE *out, FILE *err)
{
	int ret;
	nls_node *tree;
	nls_node **item, *tmp;

	yyin  = in;
	yyout = out;
	nls_sys_out = out;
	nls_sys_err = err;

	nls_mem_chain_init();
	ret = yyparse();
	tree = nls_sys_parse_result; /* pointer grabbed in yyparse(). */
	if (ret || !tree) {
		goto free_exit;
	}
	ret = nls_reduce(&tree);
	nls_list_foreach(tree, &item, &tmp) {
		nls_tree_print(nls_sys_out, *item);
		fprintf(out, "\n");
	}
free_exit:
	if (tree) {
		nls_node_release(tree);
	}
	nls_mem_chain_term();
	return ret;
}

/**
 * [DESTRUCTIVE] Recursively do application.
 * @param  tree Target syntax tree.
 * @retval 0    Application succeed.
 * @retval else Error code.
 */
int
nls_reduce(nls_node **tree)
{
	int ret;

	switch ((*tree)->nn_type) {
	case NLS_TYPE_INT:
	case NLS_TYPE_VAR:
	case NLS_TYPE_FUNCTION:
	case NLS_TYPE_ABSTRACTION:
		return 0;
	case NLS_TYPE_APPLICATION:
		if ((ret = nls_apply(tree))) {
			return ret;
		}
		return nls_reduce(tree);
	case NLS_TYPE_LIST:
		return nls_list_reduce(tree);
	default:
		NLS_BUG(NLS_MSG_INVALID_NODE_TYPE ": type=%d",
			(*tree)->nn_type);
		return EINVAL; /* must not happen */
	}
}

nls_string*
nls_string_new(char *s)
{
	int hash;
	char *buf;
	nls_string *str;
	size_t len = nls_strnlen_hash(s, SIZE_MAX-1, &hash);

	if (!(buf = nls_array_new(char, len+1))) {
		return NULL;
	}
	if (!(str = nls_new(nls_string))) {
		if (buf) {
			nls_free(buf);
		}
		return NULL;
	}
	strncpy(buf, s, len);
	buf[len] = '\0';
	str->ns_ref = 0;
	str->ns_len = len;
	str->ns_hash = hash;
	str->ns_bufp = nls_grab(buf);
	return str;
}

nls_string*
nls_string_grab(nls_string *str)
{
	str->ns_ref++;
	return nls_grab(str);
}

void
nls_string_release(nls_string *str)
{
	if (--(str->ns_ref) > 0) {
		nls_release(str);
		return;
	}
	nls_release(str->ns_bufp);
	nls_release(str);
}

#ifdef NLS_UNIT_TEST
static void
test_nls_string_release(void)
{
	nls_mem *mem;
	nls_string *str, *ref1, *ref2;

	nls_sys_err = stderr;
	nls_mem_chain_init();

	str = nls_string_new("abc");
	mem = (nls_mem*)str - 1;
	NLS_ASSERT_EQUALS(0, str->ns_ref);
	NLS_ASSERT_EQUALS(0, mem->nm_ref);

	ref1 = nls_string_grab(str);
	NLS_ASSERT_EQUALS(str, ref1);
	NLS_ASSERT_EQUALS(1, str->ns_ref);
	NLS_ASSERT_EQUALS(1, mem->nm_ref);

	ref2 = nls_string_grab(str);
	NLS_ASSERT_EQUALS(str, ref2);
	NLS_ASSERT_EQUALS(2, str->ns_ref);
	NLS_ASSERT_EQUALS(2, mem->nm_ref);

	nls_string_release(ref2);
	NLS_ASSERT_EQUALS(1, str->ns_ref);
	NLS_ASSERT_EQUALS(1, mem->nm_ref);

	nls_string_release(ref1); /* free() called. */

	nls_mem_chain_term();
}
#endif /* NLS_UNIT_TEST */

void
nls_string_free(nls_string *str)
{
	if (str->ns_ref) {
		NLS_BUG(NLS_MSG_INVALID_REFCOUNT);
		return;
	}
	nls_release(str->ns_bufp);
	nls_free(str);
}

#ifdef NLS_UNIT_TEST
static void
test_nls_string_free(void)
{
	nls_string *str;

	nls_sys_err = stderr;
	nls_mem_chain_init();

	str = nls_string_new("XYZ");
	NLS_ASSERT_EQUALS(0, str->ns_ref);

	nls_string_free(str); /* free() called. */

	nls_mem_chain_term();
}
#endif /* NLS_UNIT_TEST */

static size_t
nls_strnlen_hash(char *str, size_t n, int *hashp)
{
	size_t i;
	int hash = 0;

	for (i = 0; i < n; i++, str++) {
		if ('\0' == *str) {
			break;
		}
		hash += *str;
		hash %= NLS_HASH_WIDTH;
	}
	*hashp = hash;
	return i;
}

#ifdef NLS_UNIT_TEST
static void
test_nls_strnlen_hash_when_empty(void)
{
	int hash;
	size_t len = nls_strnlen_hash("", 10, &hash);

	NLS_ASSERT_EQUALS(0, len);
	NLS_ASSERT_EQUALS(0, hash);
}

static void
test_nls_strnlen_hash_when_length1(void)
{
	int hash;
	size_t len = nls_strnlen_hash("x", 10, &hash);

	NLS_ASSERT_EQUALS(1, len);
	NLS_ASSERT_EQUALS(('x' % NLS_HASH_WIDTH), hash);
}

static void
test_nls_strnlen_hash_differ(void)
{
	int hash1, hash2;

	nls_strnlen_hash("a", 10, &hash1);
	nls_strnlen_hash("b", 10, &hash2);
	NLS_ASSERT_NOT_EQUALS(hash1, hash2);

	nls_strnlen_hash("a", 10, &hash1);
	nls_strnlen_hash("c", 10, &hash2);
	NLS_ASSERT_NOT_EQUALS(hash1, hash2);

	nls_strnlen_hash("a", 10, &hash1);
	nls_strnlen_hash("A", 10, &hash2);
	NLS_ASSERT_NOT_EQUALS(hash1, hash2);

	nls_strnlen_hash("m", 10, &hash1);
	nls_strnlen_hash("n", 10, &hash2);
	NLS_ASSERT_NOT_EQUALS(hash1, hash2);

	nls_strnlen_hash("m", 10, &hash1);
	nls_strnlen_hash("o", 10, &hash2);
	NLS_ASSERT_NOT_EQUALS(hash1, hash2);

	nls_strnlen_hash("m", 10, &hash1);
	nls_strnlen_hash("M", 10, &hash2);
	NLS_ASSERT_NOT_EQUALS(hash1, hash2);

	nls_strnlen_hash("x", 10, &hash1);
	nls_strnlen_hash("y", 10, &hash2);
	NLS_ASSERT_NOT_EQUALS(hash1, hash2);

	nls_strnlen_hash("x", 10, &hash1);
	nls_strnlen_hash("z", 10, &hash2);
	NLS_ASSERT_NOT_EQUALS(hash1, hash2);

	nls_strnlen_hash("x", 10, &hash1);
	nls_strnlen_hash("X", 10, &hash2);
	NLS_ASSERT_NOT_EQUALS(hash1, hash2);
}

static void
test_nls_strnlen_hash_when_normal(void)
{
	int hash, hash1, hash2;
	size_t len  = nls_strnlen_hash("abc", 10, &hash);

	nls_strnlen_hash("a",   10, &hash1);
	nls_strnlen_hash("bc",  10, &hash2);

	NLS_ASSERT_EQUALS(3, len);
	NLS_ASSERT_EQUALS(hash, (hash1 + hash2) % NLS_HASH_WIDTH);
}

static void
test_nls_strnlen_hash_when_over_limit(void)
{
	int hash1, hash2;
	size_t len1 = nls_strnlen_hash("ABCDEF", 5, &hash1);
	size_t len2 = nls_strnlen_hash("ABCDE",  5, &hash2);

	NLS_ASSERT_EQUALS(5, len1);
	NLS_ASSERT_EQUALS(5, len2);
	NLS_ASSERT_EQUALS(hash2, hash1);
}

static void
test_nls_strnlen_hash_when_long(void)
{
	int hash1, hash2, hash3;

	nls_strnlen_hash("WXYZabcDEFqpo", 30, &hash1);
	nls_strnlen_hash("WXYZ",          30, &hash2);
	nls_strnlen_hash("abcDEFqpo",     30, &hash3);

	NLS_ASSERT_EQUALS(hash1, (hash2 + hash3) % NLS_HASH_WIDTH);
}

static void
test_nls_strnlen_hash_when_limit_zero(void)
{
	int hash;
	size_t len = nls_strnlen_hash("x", 0, &hash);

	NLS_ASSERT_EQUALS(0, len);
	NLS_ASSERT_EQUALS(0, hash);
}
#endif /* NLS_UNIT_TEST */

static int
nls_list_reduce(nls_node **list)
{
	int ret;
	nls_node **item, *tmp;

	nls_list_foreach(*list, &item, &tmp) {
		ret = nls_reduce(item);
		if (ret) {
			return ret;
		}
	}
	return 0;
}

/*
 * @return positive or 0: evaluation result
 * 	   negative	: error code
 */
static int
nls_apply(nls_node **node)
{
	int ret;

	nls_application *app = &((*node)->nn_app);
	nls_node *func = app->na_func;
	nls_node **arg = &app->na_arg;

	switch (func->nn_type) {
	case NLS_TYPE_FUNCTION:
		{
			nls_node *out;
			nls_function fp = func->nn_func;

			if ((ret = (fp)(*arg, &out))) {
				return ret;
			}
			out = nls_node_grab(out);
			nls_node_release(*node);
			*node = out;
		}
		break;
	case NLS_TYPE_ABSTRACTION:
		NLS_ERROR(NLS_MSG_NOT_IMPLEMENTED);
		break;
	default:
		NLS_BUG(NLS_MSG_INVALID_NODE_TYPE
			": type=%d", func->nn_type);
	}
	return 0;
}

static int
nls_tree_print(FILE *out, nls_node *tree)
{
	int ret;

	switch (tree->nn_type) {
	case NLS_TYPE_INT:
		fprintf(out, "%d", tree->nn_int);
		return 0;
	case NLS_TYPE_VAR:
		fprintf(out, "%s", tree->nn_var.nv_name->ns_bufp);
		return 0;
	case NLS_TYPE_FUNCTION:
		fprintf(out, "func:%p", tree->nn_func);
		return 0;
	case NLS_TYPE_ABSTRACTION:
		fprintf(out, "(abst(%d): ", tree->nn_abst.nab_num_arg);
		if ((ret = nls_tree_print(out, tree->nn_abst.nab_def))) {
			return ret;
		}
		fprintf(out, ")");
		return 0;
	case NLS_TYPE_APPLICATION:
		fprintf(out, "(");
		if ((ret = nls_tree_print(out, tree->nn_app.na_func))) {
			return ret;
		}
		fprintf(out, " ");
		if ((ret = nls_tree_print(out, tree->nn_app.na_arg))) {
			return ret;
		}
		fprintf(out, ")");
		return 0;
	case NLS_TYPE_LIST:
		{
			int first = 1;
			nls_node **item, *tmp;

			fprintf(out, "[");
			nls_list_foreach(tree, &item, &tmp) {
				if (!first) {
					fprintf(out, " ");
				}
				if (first) {
					first = 0;
				}
				if ((ret = nls_tree_print(out, *item))) {
					return ret;
				}
			}
			fprintf(out, "]");
		}
		return 0;
	default:
		NLS_BUG(NLS_MSG_INVALID_NODE_TYPE ": type=%d", tree->nn_type);
		return EINVAL;
	}
}
