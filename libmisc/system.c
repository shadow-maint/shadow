/*
 * Copyright (c) 2009 -     , Peter Vrabec
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
                 const char *env[],
                 int ignore_stderr)
{
	int status = -1;
	int fd;
	pid_t pid;
	
	pid = fork();
	if (pid < 0) {
		return -1;
	}

	if (pid) {       /* Parent */
		waitpid (pid, &status, 0);
		return status;
	}

	fd = open ("/dev/null", O_RDWR);
	/* Child */
	dup2 (fd, 0);	// Close Stdin
	if (ignore_stderr) {
		dup2 (fd, 2);	// Close Stderr
	}

	execve (command, (char *const *) argv, (char *const *) env);
	fprintf (stderr, _("Failed to exec '%s'\n"), argv[0]);
	exit (-1);
}

