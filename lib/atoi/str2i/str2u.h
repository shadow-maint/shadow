// SPDX-FileCopyrightText: 2007-2009, Nicolas François
// SPDX-FileCopyrightText: 2023-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ATOI_STR2I_STR2U_H_
#define SHADOW_INCLUDE_LIB_ATOI_STR2I_STR2U_H_


#include <config.h>

#include <limits.h>
#include <stddef.h>

#include "atoi/a2i/a2u.h"
#include "attr.h"


ATTR_STRING(2) ATTR_ACCESS(write_only, 1)
inline int str2uh(unsigned short *restrict n, const char *restrict s);
ATTR_STRING(2) ATTR_ACCESS(write_only, 1)
inline int str2ui(unsigned int *restrict n, const char *restrict s);
ATTR_STRING(2) ATTR_ACCESS(write_only, 1)
inline int str2ul(unsigned long *restrict n, const char *restrict s);
ATTR_STRING(2) ATTR_ACCESS(write_only, 1)
inline int str2ull(unsigned long long *restrict n, const char *restrict s);


inline int
str2uh(unsigned short *restrict n, const char *restrict s)
{
	return a2uh(n, s, NULL, 0, 0, USHRT_MAX);
}


inline int
str2ui(unsigned int *restrict n, const char *restrict s)
{
	return a2ui(n, s, NULL, 0, 0, UINT_MAX);
}


inline int
str2ul(unsigned long *restrict n, const char *restrict s)
{
	return a2ul(n, s, NULL, 0, 0, ULONG_MAX);
}


inline int
str2ull(unsigned long long *restrict n, const char *restrict s)
{
	return a2ull(n, s, NULL, 0, 0, ULLONG_MAX);
}


#endif  // include guard
