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
#include <sys/types.h>

#include "alloc/reallocf.h"
#include "search/l/lsearch.h"
#include "shadow/grp/agetgroups.h"
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
	char    *g, *p, *dup;
	FILE *shadow_logfd = log_get_logfd();
	gid_t   *gids;
	size_t  n;

	gids = agetgroups(&n);
	if (gids == NULL)
		return -1;

	gids = REALLOCF(gids, n + strchrscnt(list, ",:") + 1, gid_t);
	if (gids == NULL)
		return -1;

	p = dup = strdup(list);
	if (dup == NULL)
		goto free_gids;

	while (NULL != (g = strsep(&p, ",:"))) {
		struct group  *grp;

		grp = getgrnam(g); /* local, no need for xgetgrnam */
		if (NULL == grp) {
			fprintf(shadow_logfd, _("Warning: unknown group %s\n"), g);
			continue;
		}

		LSEARCH(&grp->gr_gid, gids, &n);
	}
	free(dup);

	if (setgroups(n, gids) == -1) {
		fprintf(shadow_logfd, "setgroups: %s\n", strerror(errno));
		goto free_gids;
	}

	free(gids);
	return 0;

free_gids:
	free(gids);
	return -1;
}
#else				/* !USE_PAM */
extern int ISO_C_forbids_an_empty_translation_unit;
#endif				/* !USE_PAM */
