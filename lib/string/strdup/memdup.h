// SPDX-FileCopyrightText: 2025-2026, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRDUP_MEMDUP_H_
#define SHADOW_INCLUDE_LIB_STRING_STRDUP_MEMDUP_H_


#include "config.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "alloc/malloc.h"
#include "attr.h"
#include "sizeof.h"


// memdup_T - memory duplicate type-safe
#define memdup_T(p, n, T)   memdup_T_(p, n, typeas(T))
#define memdup_T_(p, n, T)                                            \
(                                                                     \
	_Generic(p, T *: (void)0, const T *: (void)0),                \
	(T *){memdup(p, (n) * sizeof(T))}                             \
)


ATTR_MALLOC(free)
inline void *memdup(const void *p, size_t size);


// memdup - memory duplicate
inline void *
memdup(const void *p, size_t size)
{
	void  *dup;

	dup = malloc(size);
	if (dup == NULL)
		return NULL;

	return memcpy(dup, p, size);
}


#endif  // include guard
