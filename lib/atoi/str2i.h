// SPDX-FileCopyrightText: 2007-2009, Nicolas François
// SPDX-FileCopyrightText: 2023-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ATOI_STR2I_H_
#define SHADOW_INCLUDE_LIB_ATOI_STR2I_H_


#include <config.h>

#include <stdlib.h>
#include <errno.h>

#include "atoi/str2i.h"
#include "atoi/strtou_noneg.h"
#include "attr.h"


ATTR_ACCESS(write_only, 2)
inline int getlong(const char *restrict s, long *restrict n);
ATTR_ACCESS(write_only, 2)
inline int getulong(const char *restrict s, unsigned long *restrict n);


inline int
getlong(const char *restrict s, long *restrict n)
{
	char  *endp;
	long  val;

	errno = 0;
	val = strtol(s, &endp, 0);
	if (('\0' == *s) || ('\0' != *endp) || (0 != errno))
		return -1;

	*n = val;
	return 0;
}


inline int
getulong(const char *restrict s, unsigned long *restrict n)
{
	char           *endp;
	unsigned long  val;

	errno = 0;
	val = strtoul_noneg(s, &endp, 0);
	if (('\0' == *s) || ('\0' != *endp) || (0 != errno))
		return -1;

	*n = val;
	return 0;
}


#endif  // include guard
