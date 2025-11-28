// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRDUP_MEMDUP_H_
#define SHADOW_INCLUDE_LIB_STRING_STRDUP_MEMDUP_H_


#include "config.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "attr.h"
#include "sizeof.h"


// memdup_T - memory duplicate type-safe
#define memdup_T(p, T)   memdup_T_(p, typeas(T))
#define memdup_T_(p, T)                                               \
({                                                                    \
	_Generic(p, T *: 0, const T *: 0);                            \
	(T *){memdup(p, sizeof(T))};                                  \
})


ATTR_MALLOC(free)
inline void *memdup(const void *p, size_t size);


// memdup - memory duplicate
inline void *
memdup(const void *p, size_t size)
{
	void  *new;

	new = malloc(size);
	if (new == NULL)
		return NULL;

	return memcpy(new, p, size);
}


#endif  // include guard
