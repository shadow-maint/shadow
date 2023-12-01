/*
 * SPDX-FileCopyrightText: 2009       , Nicolas François
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <config.h>

#ident "$Id$"

#include "prototypes.h"
#include "defines.h"

int get_gid (const char *gidstr, gid_t *gid)
{
	long long  val;
	char *endptr;

	errno = 0;
	val = strtoll(gidstr, &endptr, 10);
	if (   ('\0' == *gidstr)
	    || ('\0' != *endptr)
	    || (0 != errno)
	    || (/*@+longintegral@*/val != (gid_t)val)/*@=longintegral@*/) {
		return 0;
	}

	*gid = val;
	return 1;
}

