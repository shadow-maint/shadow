/*
 * SPDX-FileCopyrightText: 1990 - 1994, Julianne Frances Haugh
 * SPDX-FileCopyrightText: 1996 - 2000, Marek Michałkiewicz
 * SPDX-FileCopyrightText: 2001       , Michał Moskal
 * SPDX-FileCopyrightText: 2005       , Tomasz Kłoczko
 * SPDX-FileCopyrightText: 2007 - 2013, Nicolas François
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <config.h>

#ident "$Id$"

#include "prototypes.h"
#include "defines.h"
#include "groupio.h"

/*@null@*/ /*@only@*/struct group *__gr_dup (const struct group *grent)
{
	struct group *gr;
	int i;

	gr = (struct group *) malloc (sizeof *gr);
	if (NULL == gr) {
		return NULL;
	}
	/* The libc might define other fields. They won't be copied. */
	memset (gr, 0, sizeof *gr);
	gr->gr_gid = grent->gr_gid;
	/*@-mustfreeonly@*/
	gr->gr_name = strdup (grent->gr_name);
	/*@=mustfreeonly@*/
	if (NULL == gr->gr_name) {
		gr_free(gr);
		return NULL;
	}
	/*@-mustfreeonly@*/
	gr->gr_passwd = strdup (grent->gr_passwd);
	/*@=mustfreeonly@*/
	if (NULL == gr->gr_passwd) {
		gr_free(gr);
		return NULL;
	}

	for (i = 0; grent->gr_mem[i]; i++);

	/*@-mustfreeonly@*/
	gr->gr_mem = (char **) malloc ((i + 1) * sizeof (char *));
	/*@=mustfreeonly@*/
	if (NULL == gr->gr_mem) {
		gr_free(gr);
		return NULL;
	}
	for (i = 0; grent->gr_mem[i]; i++) {
		gr->gr_mem[i] = strdup (grent->gr_mem[i]);
		if (NULL == gr->gr_mem[i]) {
			gr_free(gr);
			return NULL;
		}
	}
	gr->gr_mem[i] = NULL;

	return gr;
}

void gr_free_members (struct group *grent)
{
	if (NULL != grent->gr_mem) {
		size_t i;
		for (i = 0; NULL != grent->gr_mem[i]; i++) {
			free (grent->gr_mem[i]);
		}
		free (grent->gr_mem);
		grent->gr_mem = NULL;
	}
}

void gr_free (/*@out@*/ /*@only@*/struct group *grent)
{
	free (grent->gr_name);
	if (NULL != grent->gr_passwd) {
		strzero (grent->gr_passwd);
		free (grent->gr_passwd);
	}
	gr_free_members(grent);
	free (grent);
}

bool gr_append_member(struct group *grp, char *member)
{
	int i;

	if (NULL == grp->gr_mem || grp->gr_mem[0] == NULL) {
		grp->gr_mem = (char **)malloc(2 * sizeof(char *));
		if (!grp->gr_mem) {
			return false;
		}
		grp->gr_mem[0] = strdup(member);
		if (!grp->gr_mem[0]) {
			return false;
		}
		grp->gr_mem[1] = NULL;
		return true;
	}

	for (i = 0; grp->gr_mem[i]; i++) ;
	grp->gr_mem = realloc(grp->gr_mem, (i + 2) * sizeof(char *));
	if (NULL == grp->gr_mem) {
		return false;
	}
	grp->gr_mem[i] = strdup(member);
	if (NULL == grp->gr_mem[i]) {
		return false;
	}
	grp->gr_mem[i + 1] = NULL;
	return true;
}
