/*
 * Copyright (c) 2009       , Dan Walsh <dwalsh@redhat.com>
 * Copyright (c) 2010       , Nicolas François
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
#include <sys/wait.h>
#include <fcntl.h>
#include "prototypes.h"
#include "defines.h"

int safe_system (const char *command,
                 const char *argv[],
                 /*@null@*/const char *env[],
                 bool ignore_stderr)
{
	int status = -1;
	int fd;
	pid_t pid;
	
	pid = fork();
	if (pid < 0) {
		return -1;
	}

	if (pid != 0) {       /* Parent */
		if (waitpid (pid, &status, 0) > 0) {
			return status;
		} else {
			return -1;
		}
	}

	fd = open ("/dev/null", O_RDWR);
	/* Child */
	/* Close Stdin */
	if (dup2 (fd, 0) == -1) {
		exit (EXIT_FAILURE);
	}
	if (ignore_stderr) {
		/* Close Stderr */
		if (dup2 (fd, 2) == -1) {
			exit (EXIT_FAILURE);
		}
	}

	(void) execve (command, (char *const *) argv, (char *const *) env);
	(void) fprintf (stderr, _("Failed to exec '%s'\n"), argv[0]);
	exit (EXIT_FAILURE);
}

