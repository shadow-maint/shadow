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

#if HAVE_ULIMIT_H
#include <ulimit.h>
#ifndef UL_SETFSIZE
#ifdef UL_SFILLIM
#define UL_SETFSIZE UL_SFILLIM
#else
#define UL_SETFSIZE 2
#endif
#endif
#elif HAVE_SYS_RESOURCE_H
#include <sys/time.h>		/* for struct timeval on sunos4 */
/* XXX - is the above ok or should it be <time.h> on ultrix? */
#include <sys/resource.h>
#endif
#include "prototypes.h"

int set_filesize_limit (int blocks)
{
	int ret = -1;
#if HAVE_ULIMIT_H
	if (ulimit (UL_SETFSIZE, blocks) != -1) {
		ret = 0;
	}
#elif defined(RLIMIT_FSIZE)
	struct rlimit rlimit_fsize;

	rlimit_fsize.rlim_cur = 512L * blocks;
	rlimit_fsize.rlim_max = rlimit_fsize.rlim_cur;
	ret = setrlimit (RLIMIT_FSIZE, &rlimit_fsize);
#endif

	return ret;
}

