/*
 * SPDX-FileCopyrightText: 1990 - 1994, Julianne Frances Haugh
 * SPDX-FileCopyrightText: 1996 - 1998, Marek Michałkiewicz
 * SPDX-FileCopyrightText: 2005       , Tomasz Kłoczko
 * SPDX-FileCopyrightText: 2008       , Nicolas François
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <config.h>

#ident "$Id$"

#include <stdio.h>
#include <sys/types.h>
#include <grp.h>

#include "alloc.h"
#include "defines.h"
#include "prototypes.h"

#define	NFIELDS	4

/*
 * list - turn a comma-separated string into an array of (char *)'s
 *
 *	list() converts the comma-separated list of member names into
 *	an array of character pointers.
 *
 *	WARNING: I profiled this once with and without strchr() calls
 *	and found that using a register variable and an explicit loop
 *	works best.  For large /etc/group files, this is a major win.
 *
 * FINALLY added dynamic allocation.  Still need to fix sgetsgent().
 *  --marekm
 */
static char **list (char *s)
{
	static char **members = NULL;
	static int size = 0;	/* max members + 1 */
	int i;

	i = 0;
	for (;;) {
		/* check if there is room for another pointer (to a group
		   member name, or terminating NULL).  */
		if (i >= size) {
			size = i + 100;	/* at least: i + 1 */
			members = REALLOCF(members, size, char *);
			if (!members)
				return NULL;
		}
		if (!s || s[0] == '\0')
			break;
		members[i++] = s;
		while (('\0' != *s) && (',' != *s)) {
			s++;
		}
		if ('\0' != *s) {
			*s++ = '\0';
		}
	}
	members[i] = NULL;
	return members;
}


struct group *sgetgrent (const char *buf)
{
	static char *grpbuf = NULL;
	static size_t size = 0;
	static char *grpfields[NFIELDS];
	static struct group grent;
	int i;
	char *cp;

	if (strlen (buf) + 1 > size) {
		/* no need to use realloc() here - just free it and
		   allocate a larger block */
		free (grpbuf);
		size = strlen (buf) + 1000;	/* at least: strlen(buf) + 1 */
		grpbuf = MALLOC(size, char);
		if (grpbuf == NULL) {
			size = 0;
			return NULL;
		}
	}
	strcpy (grpbuf, buf);

	cp = strrchr (grpbuf, '\n');
	if (NULL != cp) {
		*cp = '\0';
	}

	for (cp = grpbuf, i = 0; (i < NFIELDS) && (NULL != cp); i++) {
		grpfields[i] = cp;
		cp = strchr (cp, ':');
		if (NULL != cp) {
			*cp = '\0';
			cp++;
		}
	}
	if (i < (NFIELDS - 1) || *grpfields[2] == '\0' || cp != NULL) {
		return NULL;
	}
	grent.gr_name = grpfields[0];
	grent.gr_passwd = grpfields[1];
	if (get_gid(grpfields[2], &grent.gr_gid) == -1) {
		return NULL;
	}
	grent.gr_mem = list (grpfields[3]);
	if (NULL == grent.gr_mem) {
		return NULL;	/* out of memory */
	}

	return &grent;
}

