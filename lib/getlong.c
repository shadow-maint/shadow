/*
 * SPDX-FileCopyrightText: 2007 - 2009, Nicolas François
 * SPDX-FileCopyrightText: 2023, Alejandro Colomar <alx@kernel.org>
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <config.h>

#ident "$Id$"

#include <limits.h>
#include <stddef.h>
#include <stdlib.h>

#include "atoi/strtoi.h"
#include "prototypes.h"


/*
 * getlong - extract a long integer provided by the numstr string in *result
 *
 * It supports decimal, hexadecimal or octal representations.
 */
int
getlong(const char *numstr, /*@out@*/long *result)
{
	int   status;
	long  val;

	val = strtonl(numstr, NULL, 0, &status, long);
	if (status != 0)
		return -1;

	*result = val;
	return 0;
}
