// SPDX-FileCopyrightText: 2023-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ALLOC_REALLOC_H_
#define SHADOW_INCLUDE_LIB_ALLOC_REALLOC_H_


#include <config.h>

#include <stdlib.h>


#define REALLOC(p, n, type)                                                   \
(                                                                             \
	_Generic(p, type *: (type *) reallocarray(p, (n) ?: 1, sizeof(type))) \
)


#endif  // include guard
