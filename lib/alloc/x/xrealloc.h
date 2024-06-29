// SPDX-FileCopyrightText: 2023-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_MALLOC_H_
#define SHADOW_INCLUDE_LIB_MALLOC_H_


#include <config.h>

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "attr.h"


#define XREALLOC(ptr, n, type)                                                \
(                                                                             \
	_Generic(ptr, type *:  (type *) xreallocarray(ptr, n, sizeof(type)))  \
)


ATTR_ALLOC_SIZE(2, 3)
ATTR_MALLOC(free)
void *xreallocarray(void *p, size_t nmemb, size_t size);


#endif  // include guard
