// SPDX-FileCopyrightText: 2023-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ADDS_H_
#define SHADOW_INCLUDE_LIB_ADDS_H_


#include <config.h>

#include <errno.h>
#include <limits.h>
#include <stddef.h>

#include "search/sort/qsort.h"
#include "sizeof.h"


#define addsl(a, b, ...)                                                      \
({                                                                            \
	long  addend_[] = {a, b, __VA_ARGS__};                                \
                                                                              \
	addslN(countof(addend_), addend_);                                    \
})


inline long addsl2(long a, long b);
inline long addslN(size_t n, long addend[n]);


inline long
addsl2(long a, long b)
{
	if (a > 0 && b > LONG_MAX - a) {
		errno = EOVERFLOW;
		return LONG_MAX;
	}
	if (a < 0 && b < LONG_MIN - a) {
		errno = EOVERFLOW;
		return LONG_MIN;
	}
	return a + b;
}


inline long
addslN(size_t n, long addend[n])
{
	int   e;

	if (n == 0) {
		errno = EDOM;
		return 0;
	}

	e = errno;
	while (n > 1) {
		QSORT(addend, n);

		errno = 0;
		addend[0] = addsl2(addend[0], addend[--n]);
		if (errno == EOVERFLOW)
			return addend[0];
	}
	errno = e;
	return addend[0];
}


#endif  // include guard
