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
#include "search/l/lsearch.h"
#include "shadowlog.h"
#include "string/strchr/strchrscnt.h"


/*
 * Add groups with names from LIST (separated by commas or colons)
 * to the supplementary group set.  Silently ignore groups which are
 * already there.
 */
int
add_groups(const char *list)
{
	GETGROUPS_T *grouplist;
	char *g, *p;
	char buf[1024];
	FILE *shadow_logfd = log_get_logfd();
	size_t  n;
	ssize_t n0;

	if (strlen (list) >= sizeof (buf)) {
		errno = EINVAL;
		return -1;
	}
	strcpy (buf, list);

	n0 = getgroups(0, NULL);
	if (n0 == -1)
		return -1;

	grouplist = MALLOC(n0, GETGROUPS_T);
	if (grouplist == NULL)
		return -1;

	n0 = getgroups(n0, grouplist);
	if (n0 == -1)
		goto free_gids;

	grouplist = REALLOCF(grouplist, n0 + strchrscnt(list, ",:") + 1, GETGROUPS_T);
	if (grouplist == NULL)
		return -1;

	n = n0;
	p = buf;
	while (NULL != (g = strsep(&p, ",:"))) {
		struct group  *grp;

		grp = getgrnam(g); /* local, no need for xgetgrnam */
		if (NULL == grp) {
			fprintf(shadow_logfd, _("Warning: unknown group %s\n"), g);
			continue;
		}

		LSEARCH(&grp->gr_gid, grouplist, &n);
	}

	if (setgroups(n, grouplist) == -1) {
		fprintf(shadow_logfd, "setgroups: %s\n", strerror(errno));
		goto free_gids;
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
