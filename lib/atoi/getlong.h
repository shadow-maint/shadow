// SPDX-FileCopyrightText: 2007-2009, Nicolas François
// SPDX-FileCopyrightText: 2023, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ATOI_GETLONG_H_
#define SHADOW_INCLUDE_LIB_ATOI_GETLONG_H_


#include <config.h>

#include <errno.h>
#include <stdlib.h>

#include "atoi/getlong.h"
#include "atoi/strtou_noneg.h"
#include "attr.h"


ATTR_STRING(1) ATTR_ACCESS(write_only, 2)
inline int getl(const char *numstr, long *result);
ATTR_STRING(1) ATTR_ACCESS(write_only, 2)
inline int getul(const char *numstr, unsigned long *result);


/*
 * getl - extract a long integer provided by the numstr string in *result
 *
 * It supports decimal, hexadecimal or octal representations.
 */
inline int
getl(const char *numstr, long *result)
{
	char  *endptr;
	long  val;

	errno = 0;
	val = strtol(numstr, &endptr, 0);
	if (('\0' == *numstr) || ('\0' != *endptr) || (0 != errno))
		return -1;

	*result = val;
	return 0;
}


/*
 * getul - extract an unsigned long integer provided by the numstr string in *result
 *
 * It supports decimal, hexadecimal or octal representations.
 */
inline int
getul(const char *numstr, unsigned long *result)
{
	char           *endptr;
	unsigned long  val;

	errno = 0;
	val = strtoul_noneg(numstr, &endptr, 0);
	if (('\0' == *numstr) || ('\0' != *endptr) || (0 != errno))
		return -1;

	*result = val;
	return 0;
}


#endif  // include guard
