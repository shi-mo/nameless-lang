#include <stdlib.h>
#include "nameless.h"
#include "nameless/mm.h"

#define nls_mem_chain_foreach(tmp) \
	for (*(tmp) = nls_mem_chain.nm_next; \
		*(tmp) != &nls_mem_chain; \
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
	nls_mem *tmp;

	if (nls_mem_alloc_cnt != nls_mem_free_cnt) {
		NLS_BUG(NLS_BUGFMT_ILLEGAL_ALLOCNT,
			nls_mem_alloc_cnt, nls_mem_free_cnt);
		return;
	}
	nls_mem_chain_foreach(&tmp) {
		if (NLS_MAGIC_MEMCHUNK != tmp->nm_magic) {
			NLS_BUG(NLS_BUGMSG_BROKEN_MEMCHAIN);
			return;
		}
		if (tmp->nm_ref) {
			nls_error(NLS_ERRFMT_MEMLEAK_DETECTED,
				tmp, (tmp + 1), tmp->nm_ref, tmp->nm_size);
		}
		free(tmp);
	}
}

/*
 * Allocate memory & add to memchain
 *
 * Do NOT use this function directly.
 * Use nls_new(type) / nls_new_array(type) / nls_grab(ptr) instead.
 */
void*
_nls_malloc(size_t size)
{
	nls_mem *mem = malloc(size + sizeof(nls_mem));

	if (!mem) {
		return NULL;
	}
	nls_mem_alloc_cnt++;
	mem->nm_magic = NLS_MAGIC_MEMCHUNK;
	mem->nm_ref = 1;
	mem->nm_size = size;
	nls_mem_chain_add(mem);

	return ++mem;
}

void
nls_free(void *ptr)
{
	int ref;
	nls_mem *mem = (nls_mem*)(ptr - sizeof(nls_mem));

	if (&nls_mem_chain == mem) {
		NLS_BUG(NLS_BUGMSG_ILLEGAL_MEMCHAIN_OPERATION);
		return;
	}
	if (NLS_MAGIC_MEMCHUNK != mem->nm_magic) {
		nls_error(NLS_BUGMSG_BROKEN_MEMCHAIN);
		return;
	}

	ref = --(mem->nm_ref);
	if (ref < 0) {
		NLS_BUG(NLS_BUGFMT_INVALID_REFCOUNT,
			mem, (mem + 1), mem->nm_ref, mem->nm_size);
		return;
	}
	if (!ref) {
		nls_mem_free_cnt++;
		nls_mem_chain_remove(mem);
		free(mem);
	}
}

static void
nls_mem_chain_add(nls_mem *mem)
{
	nls_mem *tail = nls_mem_chain.nm_prev;

	if (&nls_mem_chain == mem) {
		NLS_BUG(NLS_BUGMSG_ILLEGAL_MEMCHAIN_OPERATION);
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
		NLS_BUG(NLS_BUGMSG_ILLEGAL_MEMCHAIN_OPERATION);
		return;
	}
	mem->nm_next->nm_prev = mem->nm_prev;
	mem->nm_prev->nm_next = mem->nm_next;
	mem->nm_next = (nls_mem*)NLS_MAGIC_MEMCHAIN_REMOVED;
	mem->nm_prev = (nls_mem*)NLS_MAGIC_MEMCHAIN_REMOVED;
}
