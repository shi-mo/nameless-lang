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
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include "nameless/mm.h"
#include "nameless/string.h"

static size_t nls_strnlen_hash(char *str, size_t n, int *hashp);

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

void
nls_string_free(void *ptr)
{
	nls_string *str = (nls_string*)ptr;

	nls_release(str->ns_bufp);
	nls_free(str);
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

	ref1 = nls_grab(str);
	NLS_ASSERT_EQUALS(str, ref1);
	NLS_ASSERT_EQUALS(1, mem->nm_ref);

	ref2 = nls_grab(str);
	NLS_ASSERT_EQUALS(str, ref2);
	NLS_ASSERT_EQUALS(2, mem->nm_ref);

	nls_release(ref2);
	NLS_ASSERT_EQUALS(1, mem->nm_ref);

	nls_release(ref1); /* free() called. */
}

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
