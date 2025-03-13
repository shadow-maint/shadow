// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_SEARCH_L_LSEARCH_H_
#define SHADOW_INCLUDE_LIB_SEARCH_L_LSEARCH_H_


#include <config.h>

#include <search.h>
#include <stddef.h>
#include <sys/types.h>

#include "search/cmp/cmp.h"


#define LSEARCH(T, ...)                                               \
(                                                                     \
	_Generic((T) 0,                                               \
		int:     LSEARCH__ ## int,                            \
		long:    LSEARCH__ ## long,                           \
		u_int:   LSEARCH__ ## u_int,                          \
		u_long:  LSEARCH__ ## u_long                          \
	)(__VA_ARGS__)                                                \
)


#define template_LSEARCH(T)                                           \
inline void                                                           \
LSEARCH__ ## T(size_t *n;                                             \
    const T *k, T a[*n], size_t *n)                                   \
{                                                                     \
	lsearch(k, a, n, sizeof(T), CMP(T));                          \
}
template_LSEARCH(int);
template_LSEARCH(long);
template_LSEARCH(u_int);
template_LSEARCH(u_long);
#undef template_LSEARCH


#endif  // include guard
