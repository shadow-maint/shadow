// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_SEARCH_L_LFIND_H_
#define SHADOW_INCLUDE_LIB_SEARCH_L_LFIND_H_


#include "config.h"

#include <search.h>
#include <stddef.h>

#include "sizeof.h"


// lfind_T - linear find type-safe
#define lfind_T(T, ...)            lfind_T_(typeas(T), __VA_ARGS__)
#define lfind_T_(T, ...)                                              \
((static inline const T *                                             \
  (size_t n;                                                          \
   const T *k, const T a[n], size_t n, int (*cmp)(const T *, const T *)))\
{                                                                     \
	/* lfind(3) wants a pointer to n for historic reasons.  */    \
	return lfind(k, a, &n, sizeof(T), cmp);                       \
}(__VA_ARGS__))


#define LFIND(T, ...)  lfind_T(T, __VA_ARGS__, CMP(T))


#endif  // include guard
