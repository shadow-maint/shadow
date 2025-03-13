// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_SEARCH_L_LFIND_H_
#define SHADOW_INCLUDE_LIB_SEARCH_L_LFIND_H_


#include "config.h"

#include <search.h>
#include <stddef.h>

#include "search/cmp/cmp.h"
#include "sizeof.h"


// lfind_T - linear find type-safe
#define lfind_T(T, ...)            lfind_T_(typeas(T), __VA_ARGS__)
#define lfind_T_(T, k, a, n, cmp)                                     \
({                                                                    \
	_Generic(k, T *: (void)0, const T *: (void)0);                \
	_Generic(a, T *: (void)0, const T *: (void)0);                \
	(const T *){lfind_(k, a, n, sizeof(T), cmp)};                 \
})

#define LFIND(T, ...)  lfind_T(T, __VA_ARGS__, CMP(T))


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
