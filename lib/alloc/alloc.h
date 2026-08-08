// SPDX-FileCopyrightText: 2023-2026, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ALLOC_ALLOC_H_
#define SHADOW_INCLUDE_LIB_ALLOC_ALLOC_H_


#include "config.h"

#include <stddef.h>
#include <stdlib.h>

#include "cast.h"
#include "exit_if_null.h"
#include "sizeof.h"


// malloc_T - malloc type-safe
#define malloc_T(n, T)        malloc_T_(n, typeas(T))
#define malloc_T_(n, T)       rvalue((T *){mallocarray(n, sizeof(T))})
// calloc_T - calloc type-safe
#define calloc_T(n, T)        calloc_T_(n, typeas(T))
#define calloc_T_(n, T)       rvalue((T *){calloc(n, sizeof(T))})
// realloc_T - realloc type-safe
#define realloc_T(p, n, T)    realloc_T_(p, n, typeas(T))
#define realloc_T_(p, n, T)                                           \
(                                                                     \
	_Generic(p, T *: (void)0),                                    \
	rvalue((T *){reallocarray_(p, n, sizeof(T))})                 \
)
// reallocf_T - realloc free-on-error type-safe
#define reallocf_T(p, n, T)   reallocf_T_(p, n, typeas(T))
#define reallocf_T_(p, n, T)                                          \
(                                                                     \
	_Generic(p, T *: (void)0),                                    \
	rvalue((T *){reallocarrayf_(p, n, sizeof(T))})                \
)


// xmalloc_T - exit-on-error malloc type-safe
#define xmalloc_T(n, T)      exit_if_null(malloc_T(n, T))
// xcalloc_T - exit-on-error calloc type-safe
#define xcalloc_T(n, T)      exit_if_null(calloc_T(n, T))
// xrealloc_T - exit-on-error realloc type-safe
#define xrealloc_T(p, n, T)  exit_if_null(realloc_T(p, n, T))


// mallocarray - malloc array
#define mallocarray(...)  reallocarray(NULL, __VA_ARGS__)


#define reallocarray_(p, n, size)   reallocarray(p, (n) ?: 1, (size) ?: 1)
#define reallocarrayf_(p, n, size)  reallocarrayf(p, (n) ?: 1, (size) ?: 1)


// reallocarrayf - realloc array free-on-error
ATTR_ALLOC_SIZE(2, 3)
ATTR_MALLOC(free)
inline void *reallocarrayf(void *p, size_t nmemb, size_t size);


inline void *
reallocarrayf(void *p, size_t nmemb, size_t size)
{
	void  *q;

	q = reallocarray(p, nmemb ?: 1, size ?: 1);

	if (q == NULL)
		free(p);
	return q;
}


#endif  // include guard
