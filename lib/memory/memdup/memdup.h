// SPDX-FileCopyrightText: 2025-2026, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_MEMORY_MEMDUP_MEMDUP_H_
#define SHADOW_INCLUDE_LIB_MEMORY_MEMDUP_MEMDUP_H_


#include "config.h"

#include <memory.h>
#include <stddef.h>
#include <stdlib.h>

#include "alloc/malloc.h"
#include "attr.h"
#include "sizeof.h"


// memdup_T - memory duplicate type-safe
#define memdup_T(p, n, T)   memdup_T_(p, n, typeas(T))
#define memdup_T_(..., T)                                             \
((static inline T *(size_t n, const T *p))                            \
{                                                                     \
	return memdup(p, n * sizeof(T));                              \
}(__VA_ARGS__))


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
