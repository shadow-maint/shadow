// SPDX-FileCopyrightText: 2023-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ALLOC_REALLOC_H_
#define SHADOW_INCLUDE_LIB_ALLOC_REALLOC_H_


#include "config.h"

#include <stdlib.h>

#include "exit_if_null.h"
#include "sizeof.h"


// realloc_T - realloc type-safe
#define realloc_T(p, n, T)  realloc_T_(typeas(T), p, n)
#define realloc_T_(T, ...)                                            \
((static inline T *(T *p, size_t n))                                  \
{                                                                     \
	return reallocarray(p, n ?: 1, sizeof(T));                    \
}(__VA_ARGS__))


// xrealloc_T - exit-on-error realloc type-safe
#define xrealloc_T(p, n, T)  exit_if_null(realloc_T(p, n, T))


#endif  // include guard
