/*
 * Copyright (c) 1989 - 1994, Julianne Frances Haugh
 * Copyright (c) 1996 - 1998, Marek Michałkiewicz
 * Copyright (c) 2001 - 2006, Tomasz Kłoczko
 * Copyright (c) 2007 - 2009, Nicolas François
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

#if defined (HAVE_SETGROUPS) && ! defined (USE_PAM)

#include "prototypes.h"
#include "defines.h"

#include <stdio.h>
#include <grp.h>
#include <errno.h>

#ident "$Id$"

#define SEP ",:"
/*
 * Add groups with names from LIST (separated by commas or colons)
 * to the supplementary group set.  Silently ignore groups which are
 * already there.  Warning: uses strtok().
 */
int add_groups (const char *list)
{
	GETGROUPS_T *grouplist, *tmp;
	size_t i;
	int ngroups;
	bool added;
	char *token;
	char buf[1024];

	if (strlen (list) >= sizeof (buf)) {
		errno = EINVAL;
		return -1;
	}
	strcpy (buf, list);

	i = 16;
	for (;;) {
		grouplist = (gid_t *) malloc (i * sizeof (GETGROUPS_T));
		if (NULL == grouplist) {
			return -1;
		}
		ngroups = getgroups (i, grouplist);
		if (   (   (-1 == ngroups)
		        && (EINVAL != errno))
		    || (i > (size_t)ngroups)) {
			/* Unexpected failure of getgroups or successful
			 * reception of the groups */
			break;
		}
		/* not enough room, so try allocating a larger buffer */
		free (grouplist);
		i *= 2;
	}
	if (ngroups < 0) {
		free (grouplist);
		return -1;
	}

	added = false;
	for (token = strtok (buf, SEP); NULL != token; token = strtok (NULL, SEP)) {
		struct group *grp;

		grp = getgrnam (token); /* local, no need for xgetgrnam */
		if (NULL == grp) {
			fprintf (stderr, _("Warning: unknown group %s\n"),
				 token);
			continue;
		}

		for (i = 0; i < (size_t)ngroups && grouplist[i] != grp->gr_gid; i++);

		if (i < (size_t)ngroups) {
			continue;
		}

		if (ngroups >= sysconf (_SC_NGROUPS_MAX)) {
			fputs (_("Warning: too many groups\n"), stderr);
			break;
		}
		tmp = (gid_t *) realloc (grouplist, (size_t)(ngroups + 1) * sizeof (GETGROUPS_T));
		if (NULL == tmp) {
			free (grouplist);
			return -1;
		}
		tmp[ngroups] = grp->gr_gid;
		ngroups++;
		grouplist = tmp;
		added = true;
	}

	if (added) {
		return setgroups ((size_t)ngroups, grouplist);
	}

	return 0;
}
#else				/* HAVE_SETGROUPS && !USE_PAM */
extern int errno;		/* warning: ANSI C forbids an empty source file */
#endif				/* HAVE_SETGROUPS && !USE_PAM */

