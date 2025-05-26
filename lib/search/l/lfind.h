// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_SEARCH_L_LFIND_H_
#define SHADOW_INCLUDE_LIB_SEARCH_L_LFIND_H_


#include <config.h>

#include <search.h>
#include <stddef.h>

#include "search/cmp/cmp.h"
#include "typetraits.h"

#include <assert.h>


#define LFIND(k, a, n)                                                \
({                                                                    \
	__auto_type  k_ = k;                                          \
	__auto_type  a_ = a;                                          \
                                                                      \
	static_assert(is_same_typeof(k_, a_), "");                    \
                                                                      \
	(typeof(k_)) lfind_(k_, a_, n, sizeof(*k_), CMP(typeof(k_))); \
})


inline void *lfind_(const void *k, const void *a, size_t n, size_t ksize,
    typeof(int (const void *k, const void *elt)) *cmp);


inline void *
lfind_(const void *k, const void *a, size_t n, size_t ksize,
    typeof(int (const void *k, const void *elt)) *cmp)
{
	// lfind(3) wants a pointer to n for historic reasons.
	return lfind(k, a, &n, ksize, cmp);
}


#endif  // include guard
