// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_SEARCH_L_LSEARCH_H_
#define SHADOW_INCLUDE_LIB_SEARCH_L_LSEARCH_H_


#include <config.h>

#include <search.h>

#include "search/cmp/cmp.h"


#define LSEARCH(T, k, a, n)  do                                       \
(                                                                     \
	const T  *k_ = k;                                             \
	T        *a_ = a;                                             \
                                                                      \
	lsearch(k_, a_, n, sizeof(T), CMP(T *));                      \
} while (0)


#endif  // include guard
