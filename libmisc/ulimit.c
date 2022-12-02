/*
 * SPDX-FileCopyrightText: 1990 - 1994, Julianne Frances Haugh
 * SPDX-FileCopyrightText: 1996 - 1997, Marek Michałkiewicz
 * SPDX-FileCopyrightText: 2003 - 2005, Tomasz Kłoczko
 * SPDX-FileCopyrightText: 2008       , Nicolas François
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <config.h>

#ident "$Id$"

#include <sys/time.h>		/* for struct timeval on sunos4 */
/* XXX - is the above ok or should it be <time.h> on ultrix? */
#include <sys/resource.h>
#include "prototypes.h"

int set_filesize_limit (int blocks)
{
	int ret = -1;
#if defined(RLIMIT_FSIZE)
	struct rlimit rlimit_fsize;

	rlimit_fsize.rlim_cur = 512L * blocks;
	rlimit_fsize.rlim_max = rlimit_fsize.rlim_cur;
	ret = setrlimit (RLIMIT_FSIZE, &rlimit_fsize);
#endif

	return ret;
}

