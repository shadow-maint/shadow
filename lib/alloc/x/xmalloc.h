// SPDX-FileCopyrightText: 2023-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ALLOC_X_XMALLOC_H_
#define SHADOW_INCLUDE_LIB_ALLOC_X_XMALLOC_H_


#include <config.h>

#include <stddef.h>

#include "alloc/x/xrealloc.h"
#include "attr.h"


#define XMALLOC(n, type)                                                      \
(                                                                             \
	(type *) xmallocarray(n, sizeof(type))                                \
)


ATTR_ALLOC_SIZE(1, 2)
ATTR_MALLOC(free)
inline void *xmallocarray(size_t nmemb, size_t size);


inline void *
xmallocarray(size_t nmemb, size_t size)
{
	return xreallocarray(NULL, nmemb, size);
}


#endif  // include guard
