// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_SEARCH_SORT_QSORT_H_
#define SHADOW_INCLUDE_LIB_SEARCH_SORT_QSORT_H_


#include <config.h>

#include <stdlib.h>

#include "search/cmp/cmp.h"
#include "typetraits.h"


#define QSORT(a, n)  do                                               \
{                                                                     \
	__auto_type  p_ = a;                                          \
                                                                      \
	qsort(p_, n, sizeof(*p_), CMP(typeof(p_)));                   \
} while (0)


#endif  // include guard
