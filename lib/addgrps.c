/*
 * SPDX-FileCopyrightText: 1989 - 1994, Julianne Frances Haugh
 * SPDX-FileCopyrightText: 1996 - 1998, Marek Michałkiewicz
 * SPDX-FileCopyrightText: 2001 - 2006, Tomasz Kłoczko
 * SPDX-FileCopyrightText: 2007 - 2009, Nicolas François
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <config.h>

#if !defined(USE_PAM)

#include "prototypes.h"
#include "defines.h"

#include <errno.h>
#include <grp.h>
#include <stdio.h>
#include <string.h>

#include "alloc/malloc.h"
#include "alloc/reallocf.h"
#include "shadowlog.h"

#ident "$Id$"

/*
 * Add groups with names from LIST (separated by commas or colons)
 * to the supplementary group set.  Silently ignore groups which are
 * already there.
 */
int
add_groups(const char *list)
{
	GETGROUPS_T *grouplist;
	int ngroups;
	bool added;
	char *g, *p;
	char buf[1024];
	FILE *shadow_logfd = log_get_logfd();

	if (strlen (list) >= sizeof (buf)) {
		errno = EINVAL;
		return -1;
	}
	strcpy (buf, list);

	for (size_t i = 16; /* void */; i *= 2) {
		grouplist = MALLOC(i, GETGROUPS_T);
		if (NULL == grouplist) {
			return -1;
		}
		ngroups = getgroups (i, grouplist);
		if (ngroups == -1 && errno != EINVAL) {
			free(grouplist);
			return -1;
		}
		if (i > (size_t)ngroups) {
			break;
		}
		free (grouplist);
	}

	added = false;
	p = buf;
	while (NULL != (g = strsep(&p, ",:"))) {
		size_t        i;
		struct group  *grp;

		grp = getgrnam(g); /* local, no need for xgetgrnam */
		if (NULL == grp) {
			fprintf(shadow_logfd, _("Warning: unknown group %s\n"), g);
			continue;
		}

		for (i = 0; i < (size_t)ngroups && grouplist[i] != grp->gr_gid; i++);

		if (i < (size_t)ngroups) {
			continue;
		}

		if (ngroups >= sysconf (_SC_NGROUPS_MAX)) {
			fputs (_("Warning: too many groups\n"), shadow_logfd);
			break;
		}
		grouplist = REALLOCF(grouplist, (size_t) ngroups + 1, GETGROUPS_T);
		if (grouplist == NULL) {
			return -1;
		}
		grouplist[ngroups] = grp->gr_gid;
		ngroups++;
		added = true;
	}

	if (added) {
		int  ret;

		ret = setgroups (ngroups, grouplist);
		free (grouplist);
		return ret;
	}

	free (grouplist);
	return 0;
}
#else				/* !USE_PAM */
extern int ISO_C_forbids_an_empty_translation_unit;
#endif				/* !USE_PAM */
