/*
 * SPDX-FileCopyrightText: 2007 - 2009, Nicolas François
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <config.h>

#ident "$Id: getlong.c 2763 2009-04-23 09:57:03Z nekral-guest $"

#include <stdlib.h>
#include <errno.h>

#include "atoi/strtou_noneg.h"
#include "prototypes.h"


/*
 * getulong - extract an unsigned long integer provided by the numstr string in *result
 *
 * It supports decimal, hexadecimal or octal representations.
 */
int
getulong(const char *restrict numstr, unsigned long *restrict result)
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
