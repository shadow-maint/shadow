/*
 * Copyright (c) 1989 - 1991, Julianne Frances Haugh
 * Copyright (c) 1996 - 1998, Marek Michałkiewicz
 * Copyright (c) 2003 - 2006, Tomasz Kłoczko
 * Copyright (c) 2009       , Nicolas François
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

#include <stdio.h>
#include <errno.h>
#include "prototypes.h"
#include "defines.h"
extern char **newenvp;
extern size_t newenvc;

/*
 * shell - execute the named program
 *
 *	shell begins by trying to figure out what argv[0] is going to
 *	be for the named process.  The user may pass in that argument,
 *	or it will be the last pathname component of the file with a
 *	'-' prepended.
 *	Then, it executes the named file.
 */

int shell (const char *file, /*@null@*/const char *arg, char *const envp[])
{
	char arg0[1024];
	int err;

	if (file == (char *) 0) {
		errno = EINVAL;
		return errno;
	}

	/*
	 * The argv[0]'th entry is usually the path name, but
	 * for various reasons the invoker may want to override
	 * that.  So, we determine the 0'th entry only if they
	 * don't want to tell us what it is themselves.
	 */
	if (arg == (char *) 0) {
		snprintf (arg0, sizeof arg0, "-%s", Basename ((char *) file));
		arg = arg0;
	}

	/*
	 * First we try the direct approach.  The system should be
	 * able to figure out what we are up to without too much
	 * grief.
	 */
	execle (file, arg, (char *) 0, envp);
	err = errno;

	if (access (file, R_OK|X_OK) == 0) {
		/*
		 * Assume this is a shell script (with no shebang).
		 * Interpret it with /bin/sh
		 */
		execle ("/bin/sh", "sh", file, (char *)0, envp);
		err = errno;
	}

	/*
	 * Obviously something is really wrong - I can't figure out
	 * how to execute this stupid shell, so I might as well give
	 * up in disgust ...
	 */
	snprintf (arg0, sizeof arg0, _("Cannot execute %s"), file);
	errno = err;
	perror (arg0);
	return err;
}

