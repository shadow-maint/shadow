// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_SEARCH_L_LSEARCH_H_
#define SHADOW_INCLUDE_LIB_SEARCH_L_LSEARCH_H_


#include <config.h>

#include <search.h>

#include "search/cmp/cmp.h"
#include "typetraits.h"

#include <assert.h>


#define LSEARCH(k, a, n)                                              \
({                                                                    \
	__auto_type  k_ = k;                                          \
	__auto_type  a_ = a;                                          \
                                                                      \
	static_assert(is_same_typeof(k_, a_), "");                    \
                                                                      \
	(typeof(k_)) lsearch(k_, a_, n, sizeof(*k_), CMP(typeof(k_)));\
})


#endif  // include guard
