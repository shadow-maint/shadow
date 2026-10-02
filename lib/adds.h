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


#define adds_T(T, a, b, ...)                                          \
({                                                                    \
	T       addend_[] = {(a), (b), __VA_ARGS__};                  \
	int     e_;                                                   \
	size_t  n_;                                                   \
	                                                              \
	n_ = countof(addend_);                                        \
	e_ = errno;                                                   \
	while (n_ > 1) {                                               \
		QSORT(T, addend_, n_);                                \
		                                                      \
		errno = 0;                                            \
		addend_[0] = adds2_T_(T, addend_[0], addend_[--n_]);  \
		if (errno != 0)                                       \
			break;                                        \
	}                                                             \
	if (errno == 0)                                               \
		errno = e_;                                           \
	                                                              \
	addend_[0];                                                   \
})


#endif  // include guard
