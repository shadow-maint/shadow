/*
 * SPDX-FileCopyrightText: 2007 - 2009, Nicolas François
 * SPDX-FileCopyrightText: 2023, Alejandro Colomar <alx@kernel.org>
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <config.h>

#ident "$Id: getlong.c 2763 2009-04-23 09:57:03Z nekral-guest $"

#include <limits.h>
#include <stddef.h>
#include <stdlib.h>

#include "atoi/strtoi.h"
#include "prototypes.h"


/*
 * getulong - extract an unsigned long integer provided by the numstr string in *result
 *
 * It supports decimal, hexadecimal or octal representations.
 */
int
getulong(const char *numstr, /*@out@*/unsigned long *result)
{
	int            status;
	unsigned long  val;

	val = strtonl(numstr, NULL, 0, &status, unsigned long);
	if (status != 0)
		return -1;

	*result = val;
	return 0;
}
