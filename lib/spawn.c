/*
 * Copyright (c) 2011, Jonathan Nieder
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE FREEBSD
 * PROJECT OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <config.h>

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include "exitcodes.h"
#include "spawn.h"

extern char **environ;

int run_command (const char *cmd, const char *argv[], const char *envp[],
                 int *status)
{
	pid_t pid, wpid;

	if (!envp)
		envp = (const char **)environ;
	pid = fork ();
	if (pid == 0) {
		execve (cmd, (char * const *) argv, (char * const *) envp);
		if (errno == ENOENT)
			exit (E_CMD_NOTFOUND);
		perror(cmd);
		exit (E_CMD_NOEXEC);
	} else if ((pid_t)-1 == pid) {
		int saved_errno = errno;
		perror ("fork");
		errno = saved_errno;
		return -1;
	}

	do {
		wpid = waitpid (pid, status, 0);
	} while ((pid_t)-1 == wpid && errno == EINTR);

	if ((pid_t)-1 == wpid) {
		int saved_errno = errno;
		perror ("waitpid");
		return -1;
	} else if (wpid != pid) {
		(void) fprintf (stderr, "waitpid returned %ld != %ld\n",
		                (long int) wpid, (long int) pid);
		errno = ECHILD;
		return -1;
	}
	return 0;
}
