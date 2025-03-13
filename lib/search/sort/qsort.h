// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_SEARCH_SORT_QSORT_H_
#define SHADOW_INCLUDE_LIB_SEARCH_SORT_QSORT_H_


#include <config.h>

#include <stdlib.h>
#include <stddef.h>
#include <sys/types.h>

#include "search/cmp/cmp.h"
#include "typetraits.h"


#define QSORT(T, ...)                                                 \
(                                                                     \
	_Generic((T) 0,                                               \
		int:     QSORT__ ## int,                              \
		long:    QSORT__ ## long,                             \
		u_int:   QSORT__ ## u_int,                            \
		u_long:  QSORT__ ## u_long                            \
	)(__VA_ARGS__)                                                \
)


#define template_QSORT(T)                                             \
inline void                                                           \
QSORT__ ## T(size_t n;                                                \
    T a[n], size_t n)                                                 \
{                                                                     \
	qsort(a, n, sizeof(T), CMP(T));                               \
}
template_QSORT(int);
template_QSORT(long);
template_QSORT(u_int);
template_QSORT(u_long);
#undef template_QSORT


#endif  // include guard
