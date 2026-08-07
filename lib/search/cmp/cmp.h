// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_SEARCH_CMP_CMP_H_
#define SHADOW_INCLUDE_LIB_SEARCH_CMP_CMP_H_


#include "config.h"


/* Compatible with bsearch(3), lfind(3), and qsort(3).  */
#define CMP(T)                                                        \
((static inline int (const T *key, const T *elt))                     \
{                                                                     \
	if (*key < *elt)                                              \
		return -1;                                            \
	if (*key > *elt)                                              \
		return +1;                                            \
	return 0;                                                     \
})


#endif  // include guard
