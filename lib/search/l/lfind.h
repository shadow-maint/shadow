// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_SEARCH_L_LFIND_H_
#define SHADOW_INCLUDE_LIB_SEARCH_L_LFIND_H_


#include <config.h>

#include <search.h>
#include <stddef.h>
#include <sys/types.h>

#include "search/cmp/cmp.h"


#define LFIND(T, ...)                                                 \
(                                                                     \
	_Generic((T) 0,                                               \
		int:     LFIND__ ## int,                              \
		long:    LFIND__ ## long,                             \
		u_int:   LFIND__ ## u_int,                            \
		u_long:  LFIND__ ## u_long                            \
	)(__VA_ARGS__)                                                \
)


#define template_LFIND(T)                                             \
inline const T *                                                      \
LFIND__ ## T(size_t n;                                                \
    const T *k, const T a[n], size_t n)                               \
{                                                                     \
	/* lfind(3) wants a pointer to n for historic reasons.  */    \
	return lfind(k, a, &n, sizeof(T), CMP(T));                    \
}
template_LFIND(int);
template_LFIND(long);
template_LFIND(u_int);
template_LFIND(u_long);
#undef template_LFIND


#endif  // include guard
