/*
 * Copyright (c) 1990 - 1994, Julianne Frances Haugh
 * Copyright (c) 1996 - 2000, Marek Michałkiewicz
 * Copyright (c) 2002 - 2005, Tomasz Kłoczko
 * Copyright (c) 2007 - 2008, Nicolas François
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
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <sys/types.h>
#include "defines.h"
#include "prototypes.h"

#ifndef SUAUTHFILE
#define SUAUTHFILE "/etc/suauth"
#endif

#define	NOACTION	0
#define	NOPWORD		1
#define	DENY		-1
#define	OWNPWORD	2

#ifdef SU_ACCESS

/* Really, I could do with a few const char's here defining all the 
 * strings output to the user or the syslog. -- chris
 */
static int applies (const char *, char *);

static int isgrp (const char *, const char *);

static int lines = 0;


int check_su_auth (const char *actual_id,
                   const char *wanted_id,
                   bool su_to_root)
{
	int posn, endline;
	const char field[] = ":";
	FILE *authfile_fd;
	char temp[1024];
	char *to_users;
	char *from_users;
	char *action;

	if (!(authfile_fd = fopen (SUAUTHFILE, "r"))) {
		int err = errno;
		/*
		 * If the file doesn't exist - default to the standard su
		 * behaviour (no access control).  If open fails for some
		 * other reason - maybe someone is trying to fool us with
		 * file descriptors limit etc., so deny access.  --marekm
		 */
		if (ENOENT == err) {
			return NOACTION;
		}
		SYSLOG ((LOG_ERR,
		         "could not open/read config file '%s': %s\n",
		         SUAUTHFILE, strerror (err)));
		return DENY;
	}

	while (fgets (temp, sizeof (temp), authfile_fd) != NULL) {
		lines++;

		if (temp[endline = strlen (temp) - 1] != '\n') {
			SYSLOG ((LOG_ERR,
				 "%s, line %d: line too long or missing newline",
				 SUAUTHFILE, lines));
			continue;
		}

		while (endline > 0 && (temp[endline - 1] == ' '
				       || temp[endline - 1] == '\t'
				       || temp[endline - 1] == '\n'))
			endline--;
		temp[endline] = '\0';

		posn = 0;
		while (temp[posn] == ' ' || temp[posn] == '\t')
			posn++;

		if (temp[posn] == '\n' || temp[posn] == '#'
		    || temp[posn] == '\0') {
			continue;
		}
		if (!(to_users = strtok (temp + posn, field))
		    || !(from_users = strtok ((char *) NULL, field))
		    || !(action = strtok ((char *) NULL, field))
		    || strtok ((char *) NULL, field)) {
			SYSLOG ((LOG_ERR,
				 "%s, line %d. Bad number of fields.\n",
				 SUAUTHFILE, lines));
			continue;
		}

		if (!applies (wanted_id, to_users))
			continue;
		if (!applies (actual_id, from_users))
			continue;
		if (!strcmp (action, "DENY")) {
			SYSLOG ((su_to_root ? LOG_WARN : LOG_NOTICE,
				 "DENIED su from '%s' to '%s' (%s)\n",
				 actual_id, wanted_id, SUAUTHFILE));
			fputs (_("Access to su to that account DENIED.\n"),
			       stderr);
			fclose (authfile_fd);
			return DENY;
		} else if (!strcmp (action, "NOPASS")) {
			SYSLOG ((su_to_root ? LOG_NOTICE : LOG_INFO,
				 "NO password asked for su from '%s' to '%s' (%s)\n",
				 actual_id, wanted_id, SUAUTHFILE));
			fputs (_("Password authentication bypassed.\n"),stderr);
			fclose (authfile_fd);
			return NOPWORD;
		} else if (!strcmp (action, "OWNPASS")) {
			SYSLOG ((su_to_root ? LOG_NOTICE : LOG_INFO,
				 "su from '%s' to '%s': asking for user's own password (%s)\n",
				 actual_id, wanted_id, SUAUTHFILE));
			fputs (_("Please enter your OWN password as authentication.\n"),
			       stderr);
			fclose (authfile_fd);
			return OWNPWORD;
		} else {
			SYSLOG ((LOG_ERR,
				 "%s, line %d: unrecognized action!\n",
				 SUAUTHFILE, lines));
		}
	}
	fclose (authfile_fd);
	return NOACTION;
}

static int applies (const char *single, char *list)
{
	const char split[] = ", ";
	char *tok;

	int state = 0;

	for (tok = strtok (list, split); tok != NULL;
	     tok = strtok (NULL, split)) {

		if (!strcmp (tok, "ALL")) {
			if (state != 0) {
				SYSLOG ((LOG_ERR,
					 "%s, line %d: ALL in bad place\n",
					 SUAUTHFILE, lines));
				return 0;
			}
			state = 1;
		} else if (!strcmp (tok, "EXCEPT")) {
			if (state != 1) {
				SYSLOG ((LOG_ERR,
					 "%s, line %d: EXCEPT in bas place\n",
					 SUAUTHFILE, lines));
				return 0;
			}
			state = 2;
		} else if (!strcmp (tok, "GROUP")) {
			if ((state != 0) && (state != 2)) {
				SYSLOG ((LOG_ERR,
					 "%s, line %d: GROUP in bad place\n",
					 SUAUTHFILE, lines));
				return 0;
			}
			state = (state == 0) ? 3 : 4;
		} else {
			switch (state) {
			case 0:	/* No control words yet */
				if (!strcmp (tok, single))
					return 1;
				break;
			case 1:	/* An all */
				SYSLOG ((LOG_ERR,
					 "%s, line %d: expect another token after ALL\n",
					 SUAUTHFILE, lines));
				return 0;
			case 2:	/* All except */
				if (!strcmp (tok, single))
					return 0;
				break;
			case 3:	/* Group */
				if (isgrp (single, tok))
					return 1;
				break;
			case 4:	/* All except group */
				if (isgrp (single, tok))
					return 0;
				/* FALL THRU */
			}
		}
	}
	if ((state != 0) && (state != 3))
		return 1;
	return 0;
}

static int isgrp (const char *name, const char *group)
{
	struct group *grp;

	grp = getgrnam (group); /* local, no need for xgetgrnam */

	if (!grp || !grp->gr_mem)
		return 0;

	return is_on_list (grp->gr_mem, name);
}
#endif				/* SU_ACCESS */
