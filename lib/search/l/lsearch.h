// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_SEARCH_L_LSEARCH_H_
#define SHADOW_INCLUDE_LIB_SEARCH_L_LSEARCH_H_


#include "config.h"

#include <search.h>

#include "search/cmp/cmp.h"
#include "sizeof.h"


// lsearch_T - linear search type-safe
#define lsearch_T(T, k, a, n, cmp)   lsearch_T_(typeas(T), k, a, n, cmp)
#define lsearch_T_(T, k, a, n, cmp)                                   \
(                                                                     \
	(T *){lsearch((const T *){k}, (T *){a}, n, sizeof(T), cmp)}   \
)

#define LSEARCH(T, k, a, n)  lsearch_T(T, k, a, n, CMP(T))


#endif  // include guard
