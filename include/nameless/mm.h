#ifndef _NAMELESS_MM_H_
#define _NAMELESS_MM_H_

#include <stdlib.h>
#include <stdint.h>
#include "nameless.h"

/**********
 * MACROS *
 **********/
/* Constant */
#define NLS_MAGIC_MEMCHUNK         0x23153e3c /* NMlS_MeMc */
#define NLS_MAGIC_MEMCHAIN_REMOVED 0x23153c20 /* NMlS_McNO */

#define nls_new(type) ((type*)_nls_malloc(sizeof(type)))
#define nls_array_new(type, n) ((type*)_nls_malloc(sizeof(type) * n))

#define nls_mem_chain_foreach(tmp) \
	for (*(tmp) = nls_mem_chain.nm_next; \
		*(tmp) != &nls_mem_chain; \
		*(tmp) = (*(tmp))->nm_next)

/*********
 * TYPES *
 *********/
typedef struct _nls_mem {
	struct _nls_mem *nm_next;
	struct _nls_mem *nm_prev;
	uint32_t nm_magic;
	int nm_ref;
	size_t nm_size;
} nls_mem;

/********************
 * GLOBAL VARIABLES *
 ********************/
extern nls_mem nls_mem_chain;

/**************
 * PROTOTYPES *
 **************/
int  nls_mem_chain_init(void);
void nls_mem_chain_term(void);
void nls_mem_chain_add(nls_mem *mem);
void nls_mem_chain_remove(nls_mem *mem);
void* _nls_malloc(size_t size);
void nls_free(void *ptr);

#endif /* _NAMELESS_MM_H_ */