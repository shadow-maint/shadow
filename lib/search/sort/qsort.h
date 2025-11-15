// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_SEARCH_SORT_QSORT_H_
#define SHADOW_INCLUDE_LIB_SEARCH_SORT_QSORT_H_


#include "config.h"

#include <stdlib.h>

#include "search/cmp/cmp.h"
#include "sizeof.h"


// qsort_T - sort type-safe
#define qsort_T(T, a, n, cmp)  do                                     \
{                                                                     \
	qsort((typeas(T) *){a}, n, sizeof(T), cmp);                   \
} while (0)

#define QSORT(T, a, n)  qsort_T(T, a, n, CMP(T))


#endif  // include guard
