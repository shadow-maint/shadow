// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_SEARCH_SORT_QSORT_H_
#define SHADOW_INCLUDE_LIB_SEARCH_SORT_QSORT_H_


#include "config.h"

#include <stddef.h>
#include <stdlib.h>

#include "search/cmp/cmp.h"
#include "sizeof.h"


#define QSORT(T, ...)                                                 \
((static inline void                                                  \
  (size_t n;                                                          \
   T a[n], size_t n))                                                 \
{                                                                     \
	qsort(a, n, sizeof(T), CMP(T));                               \
}(__VA_ARGS__))


#endif  // include guard
