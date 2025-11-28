// SPDX-FileCopyrightText: 2023-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ALLOC_MALLOC_H_
#define SHADOW_INCLUDE_LIB_ALLOC_MALLOC_H_


#include "config.h"

#include <stdlib.h>

#include "attr.h"
#include "exit_if_null.h"
#include "sizeof.h"


// malloc_T - malloc type-safe
#define malloc_T(n, T)   malloc_T_(n, typeas(T))
#define malloc_T_(n, T)                                               \
({                                                                    \
	(T *){mallocarray(n, sizeof(T))};                             \
})


// xmalloc_T - exit-on-error malloc type-safe
#define xmalloc_T(n, T)  exit_if_null(malloc_T(n, T))


// mallocarray - malloc array
ATTR_ALLOC_SIZE(1, 2)
ATTR_MALLOC(free)
inline void *mallocarray(size_t nmemb, size_t size);


inline void *
mallocarray(size_t nmemb, size_t size)
{
	return reallocarray(NULL, nmemb, size);
}


#endif  // include guard
