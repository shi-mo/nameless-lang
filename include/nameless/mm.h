#ifndef _NAMELESS_MM_H_
#define _NAMELESS_MM_H_

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
#include <stdint.h>
#include "nameless.h"

#define NLS_MAGIC_MEMCHUNK         0x23153e3c /* NMlS_MeMc */
#define NLS_MAGIC_MEMCHAIN_REMOVED 0x23153c20 /* NMlS_McNO */

#define nls_new(type) ((type*)_nls_malloc(sizeof(type), #type))
#define nls_array_new(type, n) \
	((type*)_nls_malloc((sizeof(type) * (n)), "array:" #type))

/**
 * Structure leading dynamic data area.
 *
 *  +----------------+ <-- malloc() result
 *  |     nls_mem    |
 *  +----------------+ <- nls_new() result
 *  |                |
 *  | (dynamic data) |
 *  |                |
 *  |      ...       |
 *  +----------------+
 */
typedef struct _nls_mem {
	struct _nls_mem *nm_next;
	struct _nls_mem *nm_prev;
	uint32_t nm_magic;
	int nm_ref;
	size_t nm_size;
	const char *nm_type;
} nls_mem;

int  nls_mem_chain_init(void);
void nls_mem_chain_term(void);
void* nls_grab(void *ptr);
void nls_release(void *ptr);
void nls_free(void *ptr);
int nls_is_last_ref(void *ptr);
void* _nls_malloc(size_t size, const char *type);

#endif /* _NAMELESS_MM_H_ */
