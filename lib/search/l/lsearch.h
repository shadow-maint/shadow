// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_SEARCH_L_LSEARCH_H_
#define SHADOW_INCLUDE_LIB_SEARCH_L_LSEARCH_H_


#include "config.h"

#include <search.h>

#include "search/cmp/cmp.h"


// lsearch_T - linear search-and-insert type-safe
#define lsearch_T(T, k, a, n, cmp)                                    \
({                                                                    \
	_Generic(k, T *: 0, const T *: 0);                            \
	_Generic(a, T *: 0);                                          \
	(T *) lsearch(k, a, n, sizeof(T), cmp);                       \
})

#define LSEARCH(T, ...)  lsearch_T(T, __VA_ARGS__, CMP(T))


#endif  // include guard
