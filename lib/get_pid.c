/*
 * SPDX-FileCopyrightText: 2009       , Nicolas François
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <config.h>

#ident "$Id$"

#include "prototypes.h"
#include "defines.h"

int get_pid (const char *pidstr, pid_t *pid)
{
	long long int val;
	char *endptr;

	errno = 0;
	val = strtoll (pidstr, &endptr, 10);
	if (   ('\0' == *pidstr)
	    || ('\0' != *endptr)
	    || (ERANGE == errno)
	    || (/*@+longintegral@*/val != (pid_t)val)/*@=longintegral@*/) {
		return 0;
	}

	*pid = (pid_t)val;
	return 1;
}

