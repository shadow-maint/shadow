// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_SEARCH_L_LSEARCH_H_
#define SHADOW_INCLUDE_LIB_SEARCH_L_LSEARCH_H_


#include "config.h"

#include <search.h>
#include <stddef.h>

#include "sizeof.h"


// lsearch_T - linear search-and-insert type-safe
#define lsearch_T(T, ...)            lsearch_T_(typeas(T), __VA_ARGS__)
#define lsearch_T_(T, ...)                                            \
((static inline void                                                  \
  (size_t *n;                                                         \
   const T *k, T a[*n], size_t *n, int (*cmp)(const T *, const T *))) \
{                                                                     \
	lsearch(k, a, n, sizeof(T), cmp);                             \
}(__VA_ARGS__))

#define LSEARCH(T, ...)  lsearch_T(T, __VA_ARGS__, CMP(T))


#endif  // include guard
