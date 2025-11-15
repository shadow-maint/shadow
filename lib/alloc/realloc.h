// SPDX-FileCopyrightText: 2023-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ALLOC_REALLOC_H_
#define SHADOW_INCLUDE_LIB_ALLOC_REALLOC_H_


#include "config.h"

#include <stdlib.h>

#include "exit_if_null.h"


#define REALLOC(p, n, T)                                              \
(                                                                     \
	_Generic(p, T *: (T *) reallocarray(p, (n) ?: 1, sizeof(T)))  \
)


#define XREALLOC(p, n, T)  exit_if_null(REALLOC(p, n, T))


#endif  // include guard
