// SPDX-FileCopyrightText: 2023-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ADDS_H_
#define SHADOW_INCLUDE_LIB_ADDS_H_


#include "config.h"

#include <errno.h>
#include <limits.h>
#include <stddef.h>

#include "search/sort/qsort.h"
#include "sizeof.h"


#define addsh(a, b, ...)                                                      \
({                                                                            \
	short  addend_[] = {a, b, __VA_ARGS__};                               \
                                                                              \
	addshN(countof(addend_), addend_);                                    \
})


#define addsl(a, b, ...)                                                      \
({                                                                            \
	long  addend_[] = {a, b, __VA_ARGS__};                                \
                                                                              \
	addslN(countof(addend_), addend_);                                    \
})


inline short addsh2(short a, short b);
inline long addsl2(long a, long b);

inline short addshN(size_t n, short addend[n]);
inline long addslN(size_t n, long addend[n]);


inline short
addsh2(short a, short b)
{
	if (a > 0 && b > SHRT_MAX - a) {
		errno = EOVERFLOW;
		return SHRT_MAX;
	}
	if (a < 0 && b < SHRT_MIN - a) {
		errno = EOVERFLOW;
		return SHRT_MIN;
	}
	return a + b;
}

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


inline short
addshN(size_t n, short addend[n])
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
		addend[0] = addsh2(addend[0], addend[--n]);
		if (errno == EOVERFLOW)
			return addend[0];
	}
	errno = e;
	return addend[0];
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
		QSORT(long, addend, n);

		errno = 0;
		addend[0] = addsl2(addend[0], addend[--n]);
		if (errno == EOVERFLOW)
			return addend[0];
	}
	errno = e;
	return addend[0];
}


#endif  // include guard
