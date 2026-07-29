// SPDX-FileCopyrightText: 2023-2026, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ALLOC_MALLOC_H_
#define SHADOW_INCLUDE_LIB_ALLOC_MALLOC_H_


#include "config.h"

#include <stdlib.h>

#include "cast.h"
#include "exit_if_null.h"


// malloc_T - malloc type-safe
#define malloc_T(n, T)   ptr_cast(T, mallocarray(n, sizeof(T)))


// xmalloc_T - exit-on-error malloc type-safe
#define xmalloc_T(n, T)  exit_if_null(malloc_T(n, T))


// mallocarray - malloc array
#define mallocarray(...)  reallocarray(NULL, __VA_ARGS__)


#endif  // include guard
