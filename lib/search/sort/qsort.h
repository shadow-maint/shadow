// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_SEARCH_SORT_QSORT_H_
#define SHADOW_INCLUDE_LIB_SEARCH_SORT_QSORT_H_


#include "config.h"

#include <stddef.h>
#include <stdlib.h>

#include "sizeof.h"


// qsort_T - sort type-safe
#define qsort_T(T, ...)         qsort_T_(typeas(T), __VA_ARGS__)
#define qsort_T_(T, ...)                                              \
((static inline void                                                  \
  (size_t n;                                                          \
   T a[n], size_t n, int (*cmp)(const T *, const T *)))               \
{                                                                     \
	qsort(a, n, sizeof(T), cmp);                                  \
}(__VA_ARGS__))

#define QSORT(T, ...)  qsort_T(T, __VA_ARGS__, CMP(T))


#endif  // include guard
