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
#include <stdlib.h>
#include <errno.h>
#include "nameless.h"
#include "nameless/mm.h"

#define NLS_MSG_MEMLEAK_DETECTED "Memory leak detected"
#define NLS_MSG_BROKEN_MEMCHAIN  "Broken memchain"
#define NLS_MSG_ILLEGAL_ALLOCCNT "Illegal alloc count"
#define NLS_MSG_ILLEGAL_MEMCHAIN_OPERATION "Illegal memchain operation"
#define NLS_MSG_GRAB_NULL    "Grabbing NULL pointer"
#define NLS_MSG_RELEASE_NULL "Releasing NULL pointer"
#define NLS_MSG_FREE_NULL    "Freeing NULL pointer"

#define nls_mem_chain_foreach_safe(item, tmp) \
	for (*(item) = nls_mem_chain.nm_next, \
		*(tmp) = (*(item))->nm_next; \
		*(item) != &nls_mem_chain; \
		*(item) = *(tmp), \
		*(tmp) = (*(tmp))->nm_next)

static int nls_mem_alloc_cnt;
static int nls_mem_free_cnt;
static nls_mem nls_mem_chain;

static void nls_mem_chain_add(nls_mem *mem);
static void nls_mem_chain_remove(nls_mem *mem);

int
nls_mem_chain_init(void)
{
	nls_mem_alloc_cnt = 0;
	nls_mem_free_cnt  = 0;

	nls_mem_chain.nm_next = &nls_mem_chain;
	nls_mem_chain.nm_prev = &nls_mem_chain;
	nls_mem_chain.nm_ref  = 1;
	nls_mem_chain.nm_size = 0;

	return 0;
}

void
nls_mem_chain_term(void)
{
	nls_mem *item, *tmp;

	if (nls_mem_alloc_cnt != nls_mem_free_cnt) {
		NLS_WARN(NLS_MSG_ILLEGAL_ALLOCCNT ": alloc=%d free=%d",
			nls_mem_alloc_cnt, nls_mem_free_cnt);
	}
	nls_mem_chain_foreach_safe(&item, &tmp) {
		if (NLS_MAGIC_MEMCHUNK != item->nm_magic) {
			NLS_BUG(NLS_MSG_BROKEN_MEMCHAIN);
			return;
		}
		if (item->nm_ref) {
			NLS_WARN(NLS_MSG_MEMLEAK_DETECTED \
				": mem=%p ptr=%p ref=%d size=%d type=%s",
				item, (item + 1), item->nm_ref, item->nm_size,
				item->nm_type);
		}
		free(item);
	}
}

void*
nls_grab(void *ptr)
{
	nls_mem *mem;

	if (!ptr) {
		NLS_BUG(NLS_MSG_GRAB_NULL);
		return NULL;
	}
	mem = (nls_mem*)(ptr - sizeof(nls_mem));
	mem->nm_ref++;
	return ptr;
}

#ifdef NLS_UNIT_TEST
#include "nameless/node.h"

static void
test_nls_grab(void)
{
	nls_mem *mem;
	nls_node *node, *ref1, *ref2;

	node = nls_int_new(5);
	mem = (nls_mem*)node - 1;
	NLS_ASSERT_EQUALS(0, mem->nm_ref);

	ref1 = nls_grab(node);
	NLS_ASSERT_EQUALS(node, ref1);
	NLS_ASSERT_EQUALS(1, mem->nm_ref);

	ref2 = nls_grab(node);
	NLS_ASSERT_EQUALS(2, mem->nm_ref);
	nls_release(ref2);
	NLS_ASSERT_EQUALS(1, mem->nm_ref);

	nls_release(ref1); /* free() is called. */
}
#endif /* NLS_UNIT_TEST */

void
_nls_release(void *ptr, const char *file, int line, const char *func)
{
	int ref;
	nls_mem *mem;

	if (!ptr) {
		NLS_BUG(NLS_MSG_RELEASE_NULL);
		return;
	}
	mem = (nls_mem*)(ptr - sizeof(nls_mem));
	ref = --(mem->nm_ref);
	if (ref < 0) {
		NLS_BUG(NLS_MSG_INVALID_REFCOUNT "\n"
			"\tRelease at %s:%d:%s\n"
			"\tTarget: mem=%p ptr=%p ref=%d size=%d type=%s",
			file, line, func, mem, (mem + 1), mem->nm_ref,
			mem->nm_size, mem->nm_type);
		return;
	}
	if (!ref) {
		(mem->nm_free_op)(ptr);
	}
}

void
_nls_free(void *ptr, const char *file, int line, const char *func)
{
	nls_mem *mem;

	if (!ptr) {
		NLS_BUG(NLS_MSG_FREE_NULL);
		return;
	}
	mem = (nls_mem*)(ptr - sizeof(nls_mem));
	if (&nls_mem_chain == mem) {
		NLS_BUG(NLS_MSG_ILLEGAL_MEMCHAIN_OPERATION);
		return;
	}
	if (NLS_MAGIC_MEMCHUNK != mem->nm_magic) {
		NLS_ERROR(NLS_MSG_BROKEN_MEMCHAIN);
		return;
	}

	if (mem->nm_ref) {
		NLS_BUG(NLS_MSG_INVALID_REFCOUNT "\n"
			"\tFree at file:%s line:%d function:%s\n"
			"\tTarget: mem=%p ptr=%p ref=%d size=%d type=%s",
			file, line, func, mem, (mem + 1), mem->nm_ref,
			mem->nm_size, mem->nm_type);
		return;
	}
	nls_mem_free_cnt++;
	nls_mem_chain_remove(mem);
	free(mem);
}

void
nls_array_free(void *ptr)
{
	nls_free(ptr);
}

/*
 * Allocate memory & add to memchain
 *
 * Do NOT use this function directly.
 * Use nls_new(type) / nls_new_array(type) / nls_grab(ptr) instead.
 */
void*
_nls_malloc(size_t size, const char *type, nls_free_op free_op)
{
	nls_mem *mem = malloc(size + sizeof(nls_mem));

	if (!mem) {
		return NULL;
	}
	nls_mem_alloc_cnt++;
	mem->nm_magic = NLS_MAGIC_MEMCHUNK;
	mem->nm_type = type;
	mem->nm_ref  = 0;
	mem->nm_size = size;
	mem->nm_free_op = free_op;
	nls_mem_chain_add(mem);

	return ++mem;
}

#ifdef NLS_UNIT_TEST
static void
test_nls_new(void)
{
	nls_mem *mem;
	nls_node *node;

	nls_term();

	nls_mem_chain_init();
	NLS_ASSERT_EQUALS(&nls_mem_chain, nls_mem_chain.nm_prev);
	NLS_ASSERT_EQUALS(&nls_mem_chain, nls_mem_chain.nm_next);

	node = nls_grab(nls_int_new(256));
	mem = (nls_mem*)node - 1;
	NLS_ASSERT_EQUALS(&nls_mem_chain, mem->nm_prev);
	NLS_ASSERT_EQUALS(&nls_mem_chain, mem->nm_next);
	NLS_ASSERT_EQUALS(mem, nls_mem_chain.nm_prev);
	NLS_ASSERT_EQUALS(mem, nls_mem_chain.nm_next);

	nls_release(node);
	NLS_ASSERT_EQUALS(&nls_mem_chain, nls_mem_chain.nm_prev);
	NLS_ASSERT_EQUALS(&nls_mem_chain, nls_mem_chain.nm_next);

	nls_mem_chain_term();
	nls_init(stdout, stderr);
}
#endif /* NLS_UNIT_TEST */

static void
nls_mem_chain_add(nls_mem *mem)
{
	nls_mem *tail = nls_mem_chain.nm_prev;

	if (&nls_mem_chain == mem) {
		NLS_BUG(NLS_MSG_ILLEGAL_MEMCHAIN_OPERATION);
		return;
	}
	mem->nm_next  = &nls_mem_chain;
	mem->nm_prev  = tail;
	tail->nm_next = mem;
	nls_mem_chain.nm_prev = mem;
}

static void
nls_mem_chain_remove(nls_mem *mem)
{
	if (&nls_mem_chain == mem) {
		NLS_BUG(NLS_MSG_ILLEGAL_MEMCHAIN_OPERATION);
		return;
	}
	mem->nm_next->nm_prev = mem->nm_prev;
	mem->nm_prev->nm_next = mem->nm_next;
	mem->nm_next = (nls_mem*)NLS_MAGIC_MEMCHAIN_REMOVED;
	mem->nm_prev = (nls_mem*)NLS_MAGIC_MEMCHAIN_REMOVED;
}
