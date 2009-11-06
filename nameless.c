#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "y.tab.h"
#include "nameless.h"
#include "nameless/parser.h"
#include "nameless/mm.h"
#include "nameless/function.h"

#define NLS_MSG_HASH_ENOENT "No such hash entry"

NLS_GLOBAL FILE *nls_sys_out;
NLS_GLOBAL FILE *nls_sys_err;
NLS_GLOBAL nls_hash nls_sys_sym_table;

static void nls_sym_table_init(void);
static void nls_sym_table_term(void);
static void nls_hash_init(nls_hash *hash);
static void nls_hash_term(nls_hash *hash);
static nls_hash_entry* nls_hash_entry_new(nls_string *key, nls_node *node);
static nls_hash_entry* nls_hash_entry_grab(nls_hash_entry *ent);
static void nls_hash_entry_release(nls_hash_entry *ent);
static int nls_hash_add(nls_hash *hash, nls_string *key, nls_node *item);
static int nls_hash_remove(nls_hash *hash, nls_string *key);
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

	nls_init(out, err);
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
	nls_term();
	return ret;
}

void
nls_init(FILE *out, FILE *err)
{
	nls_sys_out = out;
	nls_sys_err = err;
	nls_mem_chain_init();
	nls_sym_table_init();
}

void
nls_term(void)
{
	nls_sym_table_term();
	nls_mem_chain_term();
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
	str->ns_len  = len;
	str->ns_hash = hash;
	str->ns_bufp = nls_grab(buf);
	return str;
}

nls_string*
nls_string_grab(nls_string *str)
{
	return nls_grab(str);
}

void
nls_string_release(nls_string *str)
{
	if (!nls_is_last_ref(str)) {
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

	str = nls_string_new("abc");
	mem = (nls_mem*)str - 1;
	NLS_ASSERT_EQUALS(0, mem->nm_ref);

	ref1 = nls_string_grab(str);
	NLS_ASSERT_EQUALS(str, ref1);
	NLS_ASSERT_EQUALS(1, mem->nm_ref);

	ref2 = nls_string_grab(str);
	NLS_ASSERT_EQUALS(str, ref2);
	NLS_ASSERT_EQUALS(2, mem->nm_ref);

	nls_string_release(ref2);
	NLS_ASSERT_EQUALS(1, mem->nm_ref);

	nls_string_release(ref1); /* free() called. */
}
#endif /* NLS_UNIT_TEST */

void
nls_string_free(nls_string *str)
{
	nls_release(str->ns_bufp);
	nls_free(str);
}

#ifdef NLS_UNIT_TEST
static void
test_nls_string_free(void)
{
	nls_string *str;

	str = nls_string_new("XYZ");
	nls_string_free(str); /* Must not raise error. */
}
#endif /* NLS_UNIT_TEST */

int
nls_strcmp(nls_string *s1, nls_string *s2)
{
	if (s1->ns_hash != s2->ns_hash) {
		return s1->ns_hash - s2->ns_hash;
	}
	if (s1->ns_len != s2->ns_len) {
		return s1->ns_len - s2->ns_len;
	}
	return strncmp(s1->ns_bufp, s2->ns_bufp, s1->ns_len);
}

/**
 * Search for hash.
 * @param[in]  hash  Target hash pointer
 * @param[in]  key   Search key
 * @param[out] prevp Pointer of previous hash entry set when search success
 * @return !NULL Hash entry found
 * @return  NULL No such entry
 */
nls_hash_entry*
nls_hash_search(nls_hash *hash, nls_string *key, nls_hash_entry **prevp)
{
	nls_hash_entry *ent, *prev;
	nls_hash_entry *head = &hash->nh_table[key->ns_hash];

	for (ent = head->nhe_next, prev = head;
		ent; prev = ent, ent = ent->nhe_next) {
		if (!nls_strcmp(key, ent->nhe_key)) {
			*prevp = prev;
			return ent;
		}
	}
	*prevp = NULL;
	return NULL;
}

static void
nls_sym_table_init(void)
{
	nls_hash_init(&nls_sys_sym_table);
#define NLS_SYM_TABLE_ADD_FUNC(fp, name) \
	do { \
		nls_node *func = nls_function_new((fp), (name)); \
		nls_hash_add(&nls_sys_sym_table, func->nn_func.nf_name, func); \
	} while(0)
	NLS_SYM_TABLE_ADD_FUNC(nls_func_lambda , "lambda");
	NLS_SYM_TABLE_ADD_FUNC(nls_func_add , "+");
	NLS_SYM_TABLE_ADD_FUNC(nls_func_sub , "-");
	NLS_SYM_TABLE_ADD_FUNC(nls_func_mul , "*");
	NLS_SYM_TABLE_ADD_FUNC(nls_func_div , "/");
	NLS_SYM_TABLE_ADD_FUNC(nls_func_mod , "%");
#undef  NLS_SYM_TABLE_ADD_FUNC
}

static void
nls_sym_table_term(void)
{
	nls_hash_term(&nls_sys_sym_table);
}

static void
nls_hash_init(nls_hash *hash)
{
	int i;
	nls_hash_entry *table = hash->nh_table;

	for (i = 0; i < NLS_HASH_WIDTH; i++) {
		nls_hash_entry *ent = &table[i];

		ent->nhe_next = NULL;
		ent->nhe_key  = NULL;
		ent->nhe_node = NULL;
	}
	hash->nh_num = 0;
}

static void
nls_hash_term(nls_hash *hash)
{
	int i;
	nls_hash_entry *table = hash->nh_table;

	for (i = 0; i < NLS_HASH_WIDTH; i++) {
		if (table[i].nhe_next) {
			nls_hash_entry_release(table[i].nhe_next);
		}
		table[i].nhe_next = NULL;
	}
	hash->nh_num = 0;
}

static nls_hash_entry*
nls_hash_entry_new(nls_string *key, nls_node *node)
{
	nls_hash_entry *new = nls_new(nls_hash_entry);

	if (!new) {
		return NULL;
	}
	new->nhe_next = NULL;
	new->nhe_key  = nls_string_grab(key);
	new->nhe_node = nls_node_grab(node);
	return new;
}

static nls_hash_entry*
nls_hash_entry_grab(nls_hash_entry *ent)
{
	return nls_grab(ent);
}

static void
nls_hash_entry_release(nls_hash_entry *ent)
{
	if (!nls_is_last_ref(ent)) {
		nls_release(ent);
		return;
	}
	if (ent->nhe_next) {
		nls_hash_entry_release(ent->nhe_next);
	}
	nls_string_release(ent->nhe_key);
	nls_node_release(ent->nhe_node);
	nls_release(ent);
}

static int
nls_hash_add(nls_hash *hash, nls_string *key, nls_node *item)
{
	nls_hash_entry *ent  = nls_hash_entry_new(key, item);
	nls_hash_entry *head = &hash->nh_table[key->ns_hash];

	if (!ent) {
		return ENOMEM;
	}
	if (head->nhe_next) {
		ent->nhe_next = nls_hash_entry_grab(head->nhe_next);
		nls_hash_entry_release(head->nhe_next);
	}
	head->nhe_next = nls_hash_entry_grab(ent);
	hash->nh_num++;
	return 0;
}

#ifdef NLS_UNIT_TEST
static void
test_nls_hash_add(void)
{
	nls_hash hash;
	nls_string *key1 = nls_string_grab(nls_string_new("key1"));
	nls_string *key2 = nls_string_grab(nls_string_new("KEY2"));
	nls_node *item1 = nls_node_grab(nls_int_new(1));
	nls_node *item2 = nls_node_grab(nls_int_new(2));

	nls_hash_init(&hash);
	NLS_ASSERT_EQUALS(0, hash.nh_num);
	nls_hash_add(&hash, key1, item1);
	NLS_ASSERT_EQUALS(1, hash.nh_num);
	nls_hash_add(&hash, key2, item2);
	NLS_ASSERT_EQUALS(2, hash.nh_num);

	nls_string_release(key1);
	nls_string_release(key2);
	nls_node_release(item1);
	nls_node_release(item2);
	nls_hash_term(&hash);
}
#endif /* NLS_UNIT_TEST */

static int
nls_hash_remove(nls_hash *hash, nls_string *key)
{
	nls_hash_entry *prev;
	nls_hash_entry *ent = nls_hash_search(hash, key, &prev);

	if (!ent) {
		NLS_ERROR(NLS_MSG_HASH_ENOENT);
		return EINVAL;
	}
	if (ent->nhe_next) {
		prev->nhe_next = nls_hash_entry_grab(ent->nhe_next);
	}
	nls_hash_entry_release(ent);
	hash->nh_num--;
	return 0;
}

#ifdef NLS_UNIT_TEST
static void
test_nls_hash_remove(void)
{
	nls_hash hash;
	nls_string *key1 = nls_string_grab(nls_string_new("WWW"));
	nls_string *key2 = nls_string_grab(nls_string_new("912"));
	nls_node *item1 = nls_node_grab(nls_int_new(1));
	nls_node *item2 = nls_node_grab(nls_int_new(2));

	nls_hash_init(&hash);
	nls_hash_add(&hash, key1, item1);
	nls_hash_add(&hash, key2, item2);

	NLS_ASSERT_EQUALS(2, hash.nh_num);
	nls_hash_remove(&hash, key1);
	NLS_ASSERT_EQUALS(1, hash.nh_num);
	nls_hash_remove(&hash, key2);
	NLS_ASSERT_EQUALS(0, hash.nh_num);

	nls_string_release(key1);
	nls_string_release(key2);
	nls_node_release(item1);
	nls_node_release(item2);
	nls_hash_term(&hash);
}
#endif /* NLS_UNIT_TEST */

static size_t
nls_strnlen_hash(char *str, size_t n, int *hashp)
{
	size_t i;
	int hash = 0;

	for (i = 0; (i < n) && ('\0' != *str); i++, str++) {
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
			nls_fp fp = func->nn_func.nf_fp;

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
		fprintf(out, "%s", tree->nn_func.nf_name->ns_bufp);
		return 0;
	case NLS_TYPE_ABSTRACTION:
		fprintf(out, "(lambda");
		if ((ret = nls_tree_print(out, tree->nn_abst.nab_vars))) {
			return ret;
		}
		fprintf(out, " ");
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
