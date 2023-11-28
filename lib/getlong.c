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
 *
 * Returns 0 on failure, 1 on success.
 */
int getlong (const char *numstr, /*@out@*/long *result)
{
	long val;
	char *endptr;

	errno = 0;
	val = strtol (numstr, &endptr, 0);
	if (('\0' == *numstr) || ('\0' != *endptr) || (ERANGE == errno)) {
		return 0;
	}

	*result = val;
	return 1;
}

