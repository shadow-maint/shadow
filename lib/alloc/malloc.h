// SPDX-FileCopyrightText: 2023-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ALLOC_MALLOC_H_
#define SHADOW_INCLUDE_LIB_ALLOC_MALLOC_H_


#include <config.h>

#include <stdlib.h>

#include "attr.h"


#define MALLOC(n, type)                                                       \
(                                                                             \
	(type *) mallocarray(n, sizeof(type))                                 \
)


ATTR_ALLOC_SIZE(1, 2)
ATTR_MALLOC(free)
inline void *mallocarray(size_t nmemb, size_t size);


inline void *
mallocarray(size_t nmemb, size_t size)
{
	return reallocarray(NULL, nmemb, size);
}


#endif  // include guard
