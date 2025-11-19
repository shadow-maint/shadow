// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_SEARCH_L_LFIND_H_
#define SHADOW_INCLUDE_LIB_SEARCH_L_LFIND_H_


#include "config.h"

#include <search.h>
#include <stddef.h>

#include "search/cmp/cmp.h"


#define LFIND(T, ...)                                                 \
((static inline const T *                                             \
  (size_t n;                                                          \
   const T *k, const T a[n], size_t n))                               \
{                                                                     \
	/* lfind(3) wants a pointer to n for historic reasons.  */    \
	return lfind(k, a, &n, sizeof(T), CMP(T));                    \
}(__VA_ARGS__))


#endif  // include guard
