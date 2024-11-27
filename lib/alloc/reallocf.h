// SPDX-FileCopyrightText: 2023-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ALLOC_REALLOCF_H_
#define SHADOW_INCLUDE_LIB_ALLOC_REALLOCF_H_


#include "config.h"

#include <stddef.h>
#include <stdlib.h>

#include "attr.h"
#include "sizeof.h"


// reallocf_T - realloc free-on-error type-safe
#define reallocf_T(p, n, T)   reallocf_T_(p, n, typeas(T))
#define reallocf_T_(p, n, T)                                          \
({                                                                    \
	_Generic(p, T *: (void)0);                                    \
	(T *){reallocarrayf_(p, n, sizeof(T))};                       \
})

#define reallocarrayf_(p, n, size)  reallocarrayf(p, (n) ?: 1, (size) ?: 1)


// reallocarrayf - realloc array free-on-error
ATTR_ALLOC_SIZE(2, 3)
ATTR_MALLOC(free)
inline void *reallocarrayf(void *p, size_t n, size_t size);


inline void *
reallocarrayf(void *p, size_t n, size_t size)
{
	void  *q;

	q = reallocarray(p, n ?: 1, size ?: 1);

	if (q == NULL)
		free(p);
	return q;
}


#endif  // include guard
