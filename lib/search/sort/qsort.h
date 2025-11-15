// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_SEARCH_SORT_QSORT_H_
#define SHADOW_INCLUDE_LIB_SEARCH_SORT_QSORT_H_


#include "config.h"

#include <stdlib.h>

#include "search/cmp/cmp.h"


// qsort_T - sort type-safe
#define qsort_T(T, a, n, cmp)  do                                     \
{                                                                     \
	_Generic(a, T *: 0);                                          \
	qsort(a, n, sizeof(T), cmp);                                  \
} while (0)

#define QSORT(T, ...)  qsort_T(T, __VA_ARGS__, CMP(T))


#endif  // include guard
