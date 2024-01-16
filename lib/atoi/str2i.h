// SPDX-FileCopyrightText: 2007-2009, Nicolas François
// SPDX-FileCopyrightText: 2023, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ATOI_STR2I_H_
#define SHADOW_INCLUDE_LIB_ATOI_STR2I_H_


#include <config.h>

#include <limits.h>
#include <stddef.h>

#include "atoi/getlong.h"
#include "attr.h"


#define str2i(TYPE, ...)                                                      \
(                                                                             \
	_Generic((TYPE) 0,                                                    \
		short:              str2sh,                                   \
		int:                str2si,                                   \
		long:               str2sl,                                   \
		long long:          str2sll,                                  \
		unsigned short:     str2uh,                                   \
		unsigned int:       str2ui,                                   \
		unsigned long:      str2ul,                                   \
		unsigned long long: str2ull                                   \
	)(__VA_ARGS__)                                                        \
)


ATTR_STRING(1) ATTR_ACCESS(write_only, 2)
inline int str2sh(const char *restrict s, short *restrict n);
ATTR_STRING(1) ATTR_ACCESS(write_only, 2)
inline int str2si(const char *restrict s, int *restrict n);
ATTR_STRING(1) ATTR_ACCESS(write_only, 2)
inline int str2sl(const char *restrict s, long *restrict n);
ATTR_STRING(1) ATTR_ACCESS(write_only, 2)
inline int str2sll(const char *restrict s, long long *restrict n);
ATTR_STRING(1) ATTR_ACCESS(write_only, 2)
inline int str2uh(const char *restrict s, unsigned short *restrict n);
ATTR_STRING(1) ATTR_ACCESS(write_only, 2)
inline int str2ui(const char *restrict s, unsigned int *restrict n);
ATTR_STRING(1) ATTR_ACCESS(write_only, 2)
inline int str2ul(const char *restrict s, unsigned long *restrict n);
ATTR_STRING(1) ATTR_ACCESS(write_only, 2)
inline int str2ull(const char *restrict s, unsigned long long *restrict n);


inline int
str2sh(const char *restrict s, short *restrict n)
{
	return getshort(s, n, NULL, 0, SHRT_MIN, SHRT_MAX);
}


inline int
str2si(const char *restrict s, int *restrict n)
{
	return getint(s, n, NULL, 0, INT_MIN, INT_MAX);
}


inline int
str2sl(const char *restrict s, long *restrict n)
{
	return getlong(s, n, NULL, 0, LONG_MIN, LONG_MAX);
}


inline int
str2sll(const char *restrict s, long long *restrict n)
{
	return getllong(s, n, NULL, 0, LLONG_MIN, LLONG_MAX);
}


inline int
str2uh(const char *restrict s, unsigned short *restrict n)
{
	return getushort(s, n, NULL, 0, 0, USHRT_MAX);
}


inline int
str2ui(const char *restrict s, unsigned int *restrict n)
{
	return getuint(s, n, NULL, 0, 0, UINT_MAX);
}


inline int
str2ul(const char *restrict s, unsigned long *restrict n)
{
	return getulong(s, n, NULL, 0, 0, ULONG_MAX);
}


inline int
str2ull(const char *restrict s, unsigned long long *restrict n)
{
	return getullong(s, n, NULL, 0, 0, ULLONG_MAX);
}


#endif  // include guard
