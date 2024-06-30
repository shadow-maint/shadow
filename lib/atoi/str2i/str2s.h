// SPDX-FileCopyrightText: 2007-2009, Nicolas François
// SPDX-FileCopyrightText: 2023-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ATOI_STR2I_STR2S_H_
#define SHADOW_INCLUDE_LIB_ATOI_STR2I_STR2S_H_


#include <config.h>

#include <limits.h>
#include <stddef.h>

#include "atoi/a2i/a2s.h"
#include "attr.h"


ATTR_STRING(2) ATTR_ACCESS(write_only, 1)
inline int str2sh(short *restrict n, const char *restrict s);
ATTR_STRING(2) ATTR_ACCESS(write_only, 1)
inline int str2si(int *restrict n, const char *restrict s);
ATTR_STRING(2) ATTR_ACCESS(write_only, 1)
inline int str2sl(long *restrict n, const char *restrict s);
ATTR_STRING(2) ATTR_ACCESS(write_only, 1)
inline int str2sll(long long *restrict n, const char *restrict s);


inline int
str2sh(short *restrict n, const char *restrict s)
{
	return a2sh(n, s, NULL, 0, SHRT_MIN, SHRT_MAX);
}


inline int
str2si(int *restrict n, const char *restrict s)
{
	return a2si(n, s, NULL, 0, INT_MIN, INT_MAX);
}


inline int
str2sl(long *restrict n, const char *restrict s)
{
	return a2sl(n, s, NULL, 0, LONG_MIN, LONG_MAX);
}


inline int
str2sll(long long *restrict n, const char *restrict s)
{
	return a2sll(n, s, NULL, 0, LLONG_MIN, LLONG_MAX);
}


#endif  // include guard
