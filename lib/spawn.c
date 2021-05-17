/*
 * Copyright (c) 2011       , Jonathan Nieder
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

#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "exitcodes.h"
#include "prototypes.h"

int run_command (const char *cmd, const char *argv[],
                 /*@null@*/const char *envp[], /*@out@*/int *status)
{
	pid_t pid, wpid;

	if (NULL == envp) {
		envp = (const char **)environ;
	}

	(void) fflush (stdout);
	(void) fflush (shadow_logfd);

	pid = fork ();
	if (0 == pid) {
		(void) execve (cmd, (char * const *) argv,
		               (char * const *) envp);
		if (ENOENT == errno) {
			exit (E_CMD_NOTFOUND);
		}
		fprintf (shadow_logfd, "%s: cannot execute %s: %s\n",
		         Prog, cmd, strerror (errno));
		exit (E_CMD_NOEXEC);
	} else if ((pid_t)-1 == pid) {
		fprintf (shadow_logfd, "%s: cannot execute %s: %s\n",
		         Prog, cmd, strerror (errno));
		return -1;
	}

	do {
		wpid = waitpid (pid, status, 0);
		if ((pid_t)-1 == wpid && errno == ECHILD)
			break;
	} while (   ((pid_t)-1 == wpid && errno == EINTR)
	         || ((pid_t)-1 != wpid && wpid != pid));

	if ((pid_t)-1 == wpid) {
		fprintf (shadow_logfd, "%s: waitpid (status: %d): %s\n",
		         Prog, *status, strerror (errno));
		return -1;
	}

	return 0;
}

