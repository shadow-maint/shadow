// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_SEARCH_CMP_CMP_H_
#define SHADOW_INCLUDE_LIB_SEARCH_CMP_CMP_H_


#include <config.h>

#include <sys/types.h>


/* Compatible with bsearch(3), lfind(3), and qsort(3).  */
#define CMP(T)                                                        \
(                                                                     \
	_Generic((T) 0,                                               \
		int:     CMP__ ## int,                                \
		long:    CMP__ ## long,                               \
		u_int:   CMP__ ## u_int,                              \
		u_long:  CMP__ ## u_long                              \
	)                                                             \
)


#define template_CMP(T)                                               \
inline int                                                            \
CMP__ ## T(const void *key, const void *elt)                          \
{                                                                     \
	const T  *k = key;                                            \
	const T  *e = elt;                                            \
                                                                      \
	if (*k < *e)                                                  \
		return -1;                                            \
	if (*k > *e)                                                  \
		return +1;                                            \
	return 0;                                                     \
}
template_CMP(int);
template_CMP(long);
template_CMP(u_int);
template_CMP(u_long);
#undef template_CMP


#endif  // include guard
