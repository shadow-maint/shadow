/*
 * SPDX-FileCopyrightText: 1990 - 1994, Julianne Frances Haugh
 * SPDX-FileCopyrightText: 1997       , Marek Michałkiewicz
 * SPDX-FileCopyrightText: 2003 - 2005, Tomasz Kłoczko
 * SPDX-FileCopyrightText: 2008       , Nicolas François
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <config.h>

#ident "$Id$"

#include "defines.h"
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

#include "prototypes.h"

/*
 * pwd_init - ignore signals, and set resource limits to safe
 * values.  Call this before modifying password files, so that
 * it is less likely to fail in the middle of operation.
 */
void pwd_init (void)
{
#ifdef HAVE_SYS_RESOURCE_H
	struct rlimit rlim;

# ifdef RLIMIT_CORE
	rlim.rlim_cur = rlim.rlim_max = 0;
	setrlimit (RLIMIT_CORE, &rlim);
# endif
	rlim.rlim_cur = rlim.rlim_max = RLIM_INFINITY;
# ifdef RLIMIT_AS
	setrlimit (RLIMIT_AS, &rlim);
# endif
# ifdef RLIMIT_CPU
	setrlimit (RLIMIT_CPU, &rlim);
# endif
# ifdef RLIMIT_DATA
	setrlimit (RLIMIT_DATA, &rlim);
# endif
# ifdef RLIMIT_FSIZE
	setrlimit (RLIMIT_FSIZE, &rlim);
# endif
# ifdef RLIMIT_NOFILE
	setrlimit (RLIMIT_NOFILE, &rlim);
# endif
# ifdef RLIMIT_RSS
	setrlimit (RLIMIT_RSS, &rlim);
# endif
# ifdef RLIMIT_STACK
	setrlimit (RLIMIT_STACK, &rlim);
# endif
#else				/* !HAVE_SYS_RESOURCE_H */
	set_filesize_limit (30000);
	/* don't know how to set the other limits... */
#endif				/* !HAVE_SYS_RESOURCE_H */

	signal (SIGALRM, SIG_IGN);
	signal (SIGHUP, SIG_IGN);
	signal (SIGINT, SIG_IGN);
	signal (SIGPIPE, SIG_IGN);
	signal (SIGQUIT, SIG_IGN);
	signal (SIGTERM, SIG_IGN);
#ifdef SIGTSTP
	signal (SIGTSTP, SIG_IGN);
#endif
#ifdef SIGTTOU
	signal (SIGTTOU, SIG_IGN);
#endif

	umask (077);
}
