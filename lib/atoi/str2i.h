// SPDX-FileCopyrightText: 2007-2009, Nicolas François
// SPDX-FileCopyrightText: 2023-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ATOI_STR2I_H_
#define SHADOW_INCLUDE_LIB_ATOI_STR2I_H_


#include <config.h>

#include <limits.h>
#include <stddef.h>

#include "atoi/a2i.h"
#include "attr.h"


ATTR_STRING(1) ATTR_ACCESS(write_only, 2)
inline int getlong(const char *restrict s, long *restrict n);
ATTR_STRING(1) ATTR_ACCESS(write_only, 2)
inline int getulong(const char *restrict s, unsigned long *restrict n);


inline int
getlong(const char *restrict s, long *restrict n)
{
	return a2sl(n, s, NULL, 0, LONG_MIN, LONG_MAX);
}


inline int
getulong(const char *restrict s, unsigned long *restrict n)
{
	return a2ul(n, s, NULL, 0, 0, ULONG_MAX);
}


#endif  // include guard
