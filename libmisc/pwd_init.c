/*
 * Copyright (c) 1990 - 1994, Julianne Frances Haugh
 * Copyright (c) 1997       , Marek Michałkiewicz
 * Copyright (c) 2003 - 2005, Tomasz Kłoczko
 * Copyright (c) 2008       , Nicolas François
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the copyright holders or contributors may not be used to
 *    endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

#ifdef RLIMIT_CORE
	rlim.rlim_cur = rlim.rlim_max = 0;
	setrlimit (RLIMIT_CORE, &rlim);
#endif
	rlim.rlim_cur = rlim.rlim_max = RLIM_INFINITY;
#ifdef RLIMIT_AS
	setrlimit (RLIMIT_AS, &rlim);
#endif
#ifdef RLIMIT_CPU
	setrlimit (RLIMIT_CPU, &rlim);
#endif
#ifdef RLIMIT_DATA
	setrlimit (RLIMIT_DATA, &rlim);
#endif
#ifdef RLIMIT_FSIZE
	setrlimit (RLIMIT_FSIZE, &rlim);
#endif
#ifdef RLIMIT_NOFILE
	setrlimit (RLIMIT_NOFILE, &rlim);
#endif
#ifdef RLIMIT_RSS
	setrlimit (RLIMIT_RSS, &rlim);
#endif
#ifdef RLIMIT_STACK
	setrlimit (RLIMIT_STACK, &rlim);
#endif
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
