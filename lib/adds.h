// SPDX-FileCopyrightText: 2023, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ADDS_H_
#define SHADOW_INCLUDE_LIB_ADDS_H_


#include <config.h>

#include <errno.h>
#include <limits.h>
#include <stdlib.h>

#include "sizeof.h"


inline long addsl(long a, long b);
inline long addsl3(long a, long b, long c);

inline int cmpl(const void *p1, const void *p2);


inline long
addsl(long a, long b)
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
addsl3(long a, long b, long c)
{
	int   e;
	long  sum;
	long  n[3] = {a, b, c};

	e = errno;
	qsort(n, NITEMS(n), sizeof(n[0]), cmpl);

	errno = 0;
	sum = addsl(n[0], n[2]);
	if (errno == EOVERFLOW)
		return sum;
	errno = e;
	return addsl(sum, n[1]);
}


inline int
cmpl(const void *p1, const void *p2)
{
	const long  *l1 = p1;
	const long  *l2 = p2;

	if (*l1 < *l2)
		return -1;
	if (*l1 > *l2)
		return +1;
	return 0;
}


#endif  // include guard
