// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_SEARCH_L_LSEARCH_H_
#define SHADOW_INCLUDE_LIB_SEARCH_L_LSEARCH_H_


#include "config.h"

#include <search.h>

#include "search/cmp/cmp.h"
#include "sizeof.h"


#define LSEARCH(T, k, a, n)                                           \
({                                                                    \
	const typeas(T)  *k_ = k;                                     \
	typeas(T)        *a_ = a;                                     \
                                                                      \
	(typeas(T) *){lsearch(k_, a_, n, sizeof(T), CMP(T))};         \
})


#endif  // include guard
