#include <stdlib.h>
#include "nameless.h"
#include "nameless/mm.h"

#define NLS_MSG_MEMLEAK_DETECTED "Memory leak detected"
#define NLS_MSG_BROKEN_MEMCHAIN  "Broken memchain"
#define NLS_MSG_ILLEGAL_ALLOCCNT "Illegal alloc count"
#define NLS_MSG_INVALID_REFCOUNT "Invalid reference count"
#define NLS_MSG_ILLEGAL_MEMCHAIN_OPERATION "Illegal memchain operation"
#define NLS_MSG_GRAB_NULL "Grabbing NULL pointer"

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
static void
test_nls_grab(void)
{
	nls_mem *mem;
	nls_node *np1, *np2, *np3;

	nls_mem_chain_init();

	np1 = nls_new(nls_node);
	mem = (nls_mem*)np1 - 1;
	NLS_ASSERT_EQUALS(0, mem->nm_ref);

	np2 = nls_grab(np1);
	NLS_ASSERT_EQUALS(np1, np2);
	NLS_ASSERT_EQUALS(1, mem->nm_ref);

	np3 = nls_grab(np1);
	NLS_ASSERT_EQUALS(2, mem->nm_ref);
	nls_free(np3);
	NLS_ASSERT_EQUALS(1, mem->nm_ref);

	nls_free(np2); /* free() is called. */

	nls_mem_chain_term();
}
#endif /* NLS_UNIT_TEST */

void
nls_free(void *ptr)
{
	int ref;
	nls_mem *mem = (nls_mem*)(ptr - sizeof(nls_mem));

	if (&nls_mem_chain == mem) {
		NLS_BUG(NLS_MSG_ILLEGAL_MEMCHAIN_OPERATION);
		return;
	}
	if (NLS_MAGIC_MEMCHUNK != mem->nm_magic) {
		NLS_ERROR(NLS_MSG_BROKEN_MEMCHAIN);
		return;
	}

	ref = --(mem->nm_ref);
	if (ref < 0) {
		NLS_BUG(NLS_MSG_INVALID_REFCOUNT
			": mem=%p ptr=%p ref=%d size=%d type=%s",
			mem, (mem + 1), mem->nm_ref, mem->nm_size,
			mem->nm_type);
		return;
	}
	if (!ref) {
		nls_mem_free_cnt++;
		nls_mem_chain_remove(mem);
		free(mem);
	}
}

/*
 * Allocate memory & add to memchain
 *
 * Do NOT use this function directly.
 * Use nls_new(type) / nls_new_array(type) / nls_grab(ptr) instead.
 */
void*
_nls_malloc(size_t size, const char *type)
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
	nls_mem_chain_add(mem);

	return ++mem;
}

#ifdef NLS_UNIT_TEST
static void
test_nls_new(void)
{
	nls_mem *mem;
	nls_node *node;

	nls_mem_chain_init();
	NLS_ASSERT_EQUALS(&nls_mem_chain, nls_mem_chain.nm_prev);
	NLS_ASSERT_EQUALS(&nls_mem_chain, nls_mem_chain.nm_next);

	node = nls_grab(nls_new(nls_node));
	mem = (nls_mem*)node - 1;
	NLS_ASSERT_EQUALS(&nls_mem_chain, mem->nm_prev);
	NLS_ASSERT_EQUALS(&nls_mem_chain, mem->nm_next);
	NLS_ASSERT_EQUALS(mem, nls_mem_chain.nm_prev);
	NLS_ASSERT_EQUALS(mem, nls_mem_chain.nm_next);

	nls_free(node);
	NLS_ASSERT_EQUALS(&nls_mem_chain, nls_mem_chain.nm_prev);
	NLS_ASSERT_EQUALS(&nls_mem_chain, nls_mem_chain.nm_next);

	nls_mem_chain_term();
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
