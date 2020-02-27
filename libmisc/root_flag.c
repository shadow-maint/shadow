/*
 * Copyright (c) 2011       , Julian Pidancet
 * Copyright (c) 2011       , Nicolas François
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
#include <assert.h>
#include "defines.h"
#include "prototypes.h"
/*@-exitarg@*/
#include "exitcodes.h"

static void change_root (const char* newroot);

/*
 * process_root_flag - chroot if given the --root option
 *
 * This shall be called before accessing the passwd, group, shadow,
 * gshadow, useradd's default, login.defs files (non exhaustive list)
 * or authenticating the caller.
 *
 * The audit, syslog, or locale files shall be open before
 */
extern void process_root_flag (const char* short_opt, int argc, char **argv)
{
	/*
	 * Parse the command line options.
	 */
	int i;
	const char *newroot = NULL, *val;

	for (i = 0; i < argc; i++) {
		val = NULL;
		if (   (strcmp (argv[i], "--root") == 0)
		    || ((strncmp (argv[i], "--root=", 7) == 0)
			&& (val = argv[i] + 7))
		    || (strcmp (argv[i], short_opt) == 0)) {
			if (NULL != newroot) {
				fprintf (stderr,
				         _("%s: multiple --root options\n"),
				         Prog);
				exit (E_BAD_ARG);
			}

			if (val) {
				newroot = val;
			} else if (i + 1 == argc) {
				fprintf (stderr,
				         _("%s: option '%s' requires an argument\n"),
				         Prog, argv[i]);
				exit (E_BAD_ARG);
			} else {
				newroot = argv[++ i];
			}
		}
	}

	if (NULL != newroot) {
		change_root (newroot);
	}
}

static void change_root (const char* newroot)
{
	/* Drop privileges */
	if (   (setregid (getgid (), getgid ()) != 0)
	    || (setreuid (getuid (), getuid ()) != 0)) {
		fprintf (stderr, _("%s: failed to drop privileges (%s)\n"),
		         Prog, strerror (errno));
		exit (EXIT_FAILURE);
	}

	if ('/' != newroot[0]) {
		fprintf (stderr,
		         _("%s: invalid chroot path '%s'\n"),
		         Prog, newroot);
		exit (E_BAD_ARG);
	}

	if (access (newroot, F_OK) != 0) {
		fprintf(stderr,
		        _("%s: cannot access chroot directory %s: %s\n"),
		        Prog, newroot, strerror (errno));
		exit (E_BAD_ARG);
	}

	if (chdir (newroot) != 0) {
		fprintf(stderr,
				_("%s: cannot chdir to chroot directory %s: %s\n"),
				Prog, newroot, strerror (errno));
		exit (E_BAD_ARG);
	}

	if (chroot (newroot) != 0) {
		fprintf(stderr,
		        _("%s: unable to chroot to directory %s: %s\n"),
		        Prog, newroot, strerror (errno));
		exit (E_BAD_ARG);
	}
}

