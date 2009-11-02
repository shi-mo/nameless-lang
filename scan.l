%{
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include "y.tab.h"
#include "nameless.h"
#include "nameless/mm.h"

#define NLS_HASH_WIDTH 31
#define NLS_MSG_ILLEGAL_TOKEN "Illegal token"
#define NLS_MSG_FAILED_TO_ALLOCATE_MEMORY "Failed to allocate memory"

static nls_string* nls_string_new(char *from);
static size_t nls_strnlen_hash(char *str, size_t n, int *hashp);
static int yywrap(void);
%}
%%

"+"	{ return tOP_ADD; }
"-"	{ return tOP_SUB; }
"*"	{ return tOP_MUL; }
"/"	{ return tOP_DIV; }
"%"	{ return tOP_MOD; }
"("	{ return tLPAREN; }
")"	{ return tRPAREN; }
"["	{ return tLBRACE; }
"]"	{ return tRBRACE; }
[ \t]+	{ return tSPACE; }
\r|\n|\n\r	{ return tNEWLINE; }

[1-9][0-9]*	{
	int temp;

	sscanf(yytext, "%d", &temp);
	yylval.yst_int = temp;
	return tNUMBER;
}

[_[:alpha:]][_[:alnum:]]*	{
	nls_string *str;

	str = nls_string_new(yytext);
	if (!str) {
		NLS_ERROR(NLS_MSG_FAILED_TO_ALLOCATE_MEMORY);
		exit(1);
	}
	yylval.yst_str = str;
	return tIDENT;
}

.	{
	NLS_ERROR(NLS_MSG_ILLEGAL_TOKEN);
	exit(1);
}
%%

static nls_string*
nls_string_new(char *from)
{
	int hash;
	char *buf;
	nls_string *str;
	size_t len = nls_strnlen_hash(yytext, SIZE_MAX-1, &hash);

	if (!(buf = nls_array_new(char, len+1))) {
		return NULL;
	}
	if (!(str = nls_new(nls_string))) {
		if (buf) {
			nls_free(buf);
		}
		return NULL;
	}
	strncpy(buf, from, len);
	buf[len] = '\0';
	str->ns_len  = len;
	str->ns_hash = hash;
	str->ns_bufp = buf;
	return str;
}

static size_t
nls_strnlen_hash(char *str, size_t n, int *hashp)
{
	size_t i;
	int hash = 0;

	for (i = 0; i < n; i++, str++) {
		if ('\0' == *str) {
			break;
		}
		hash += *str % NLS_HASH_WIDTH;
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
yywrap(void)
{
	return 1;
}
