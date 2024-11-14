// SPDX-FileCopyrightText: 1989-1994, Julianne Frances Haugh
// SPDX-FileCopyrightText: 1996-1998, Marek Michałkiewicz
// SPDX-FileCopyrightText: 2001-2006, Tomasz Kłoczko
// SPDX-FileCopyrightText: 2007-2009, Nicolas François
// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


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
#include "search/l/lfind.h"
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

	ngroups = getgroups(0, NULL);
	if (ngroups == -1)
		return -1;

	grouplist = MALLOC(ngroups, GETGROUPS_T);
	if (grouplist == NULL)
		return -1;

	ngroups = getgroups(ngroups, grouplist);
	if (ngroups == -1)
		goto free_gids;

	added = false;
	p = buf;
	while (NULL != (g = strsep(&p, ",:"))) {
		struct group  *grp;

		grp = getgrnam(g); /* local, no need for xgetgrnam */
		if (NULL == grp) {
			fprintf(shadow_logfd, _("Warning: unknown group %s\n"), g);
			continue;
		}

		if (LFIND(&grp->gr_gid, grouplist, ngroups) != NULL)
			continue;

		if (ngroups >= sysconf (_SC_NGROUPS_MAX)) {
			fputs (_("Warning: too many groups\n"), shadow_logfd);
			break;
		}
		grouplist = REALLOCF(grouplist, ngroups + 1, GETGROUPS_T);
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

free_gids:
	free(grouplist);
	return -1;
}
#else				/* !USE_PAM */
extern int ISO_C_forbids_an_empty_translation_unit;
#endif				/* !USE_PAM */
