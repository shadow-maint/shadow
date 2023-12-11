/*
 * SPDX-FileCopyrightText: 2007 - 2009, Nicolas François
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <config.h>

#ident "$Id$"

#include <stdlib.h>
#include <errno.h>

#include "prototypes.h"


/*
 * getlong - extract a long integer provided by the numstr string in *result
 *
 * It supports decimal, hexadecimal or octal representations.
 */
int
getlong(const char *restrict numstr, long *restrict result)
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
