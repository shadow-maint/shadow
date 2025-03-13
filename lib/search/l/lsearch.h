// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_SEARCH_L_LSEARCH_H_
#define SHADOW_INCLUDE_LIB_SEARCH_L_LSEARCH_H_


#include "config.h"

#include <search.h>
#include <stddef.h>

#include "search/cmp/cmp.h"


#define LSEARCH(T, ...)                                               \
((static inline void                                                  \
  (size_t *n;                                                         \
   const T *k, T a[*n], size_t *n))                                   \
{                                                                     \
	lsearch(k, a, n, sizeof(T), CMP(T));                          \
}(__VA_ARGS__))


#endif  // include guard
