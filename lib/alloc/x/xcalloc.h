// SPDX-FileCopyrightText: 2023-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ALLOC_X_XCALLOC_H_
#define SHADOW_INCLUDE_LIB_ALLOC_X_XCALLOC_H_


#include <config.h>

#include <stddef.h>
#include <stdlib.h>

#include "attr.h"


#define XCALLOC(n, type)                                                      \
(                                                                             \
	(type *) xcalloc(n, sizeof(type))                                     \
)


ATTR_ALLOC_SIZE(1, 2)
ATTR_MALLOC(free)
void *xcalloc(size_t nmemb, size_t size);


#endif  // include guard
