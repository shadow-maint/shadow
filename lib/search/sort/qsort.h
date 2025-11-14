// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_SEARCH_SORT_QSORT_H_
#define SHADOW_INCLUDE_LIB_SEARCH_SORT_QSORT_H_


#include "config.h"

#include <stdlib.h>

#include "search/cmp/cmp.h"


#define QSORT(T, a, n)  qsort((T *){a}, n, sizeof(T), CMP(T))


#endif  // include guard
