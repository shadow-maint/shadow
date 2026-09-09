// SPDX-FileCopyrightText: 2023-2026, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ADDS_H_
#define SHADOW_INCLUDE_LIB_ADDS_H_


#include "config.h"

#include <errno.h>
#include <limits.h>
#include <stddef.h>

#include "search/sort/qsort.h"
#include "sizeof.h"
#include "typetraits.h"


#define adds_T(T, a, b, ...)                                          \
({                                                                    \
	T  addend_[] = {a, b, __VA_ARGS__};                           \
                                                                      \
	addsN_T_(T, countof(addend_), addend_);                       \
})


#define adds2_T_(T, a, b)                                             \
({                                                                    \
	T  sum_;                                                      \
	                                                              \
	if (a > 0 && b > maxof(T) - a) {                              \
		errno = EOVERFLOW;                                    \
		sum_ = maxof(T);                                      \
	} else if (a < 0 && b < minof(T) - a) {                       \
		errno = EOVERFLOW;                                    \
		sum_ = minof(T);                                      \
	} else {                                                      \
		sum_ = a + b;                                         \
	}                                                             \
	sum_;                                                         \
})


#define addsN_T_(T, n, addend)                                        \
({                                                                    \
	int  e_;                                                      \
	                                                              \
	e_ = errno;                                                   \
	while (n > 1) {                                               \
		QSORT(T, addend, n);                                  \
		                                                      \
		errno = 0;                                            \
		addend[0] = adds2_T_(T, addend[0], addend[--n]);      \
		if (errno != 0)                                       \
			break;                                        \
	}                                                             \
	if (errno == 0)                                               \
		errno = e_;                                           \
	                                                              \
	addend[0];                                                    \
})


#endif  // include guard
