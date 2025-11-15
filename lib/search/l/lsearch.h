// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_SEARCH_L_LSEARCH_H_
#define SHADOW_INCLUDE_LIB_SEARCH_L_LSEARCH_H_


#include "config.h"

#include <search.h>

#include "search/cmp/cmp.h"
#include "sizeof.h"


#define LSEARCH(T, ...)       LSEARCH_(typeas(T), __VA_ARGS__)
#define LSEARCH_(T, k, a, n)                                          \
(                                                                     \
	(T *){lsearch((const T *){k}, (T *){a}, n, sizeof(T), CMP(T))}\
)


#endif  // include guard
