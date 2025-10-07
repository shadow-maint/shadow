// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_SEARCH_L_LFIND_H_
#define SHADOW_INCLUDE_LIB_SEARCH_L_LFIND_H_


#include "config.h"

#include <search.h>
#include <stddef.h>

#include "search/cmp/cmp.h"
#include "typetraits.h"


#define LFIND(T, k, a, n)                                             \
({                                                                    \
	const T  *k_ = k;                                             \
	const T  *a_ = a;                                             \
                                                                      \
	(const T *) lfind_(k_, a_, n, sizeof(T), CMP(T *));           \
})


inline const void *lfind_(const void *k, const void *a, size_t n, size_t ksize,
    typeof(int (const void *k, const void *elt)) *cmp);


inline const void *
lfind_(const void *k, const void *a, size_t n, size_t ksize,
    typeof(int (const void *k, const void *elt)) *cmp)
{
	// lfind(3) wants a pointer to n for historic reasons.
	return lfind(k, a, &n, ksize, cmp);
}


#endif  // include guard
