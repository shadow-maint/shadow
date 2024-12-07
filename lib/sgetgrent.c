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

#include <grp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "alloc/malloc.h"
#include "alloc/reallocf.h"
#include "atoi/getnum.h"
#include "defines.h"
#include "prototypes.h"
#include "string/strcmp/streq.h"
#include "string/strtok/stpsep.h"
#include "string/strtok/strsep2arr.h"


/*
 * list - turn a comma-separated string into an array of (char *)'s
 *
 *	list() converts the comma-separated list of member names into
 *	an array of character pointers.
 *
 * FINALLY added dynamic allocation.  Still need to fix sgetsgent().
 *  --marekm
 */
static char **
list(char *s)
{
	static char **members = NULL;
	static size_t size = 0;	/* max members + 1 */
	size_t i;

	free(members);
	members = NULL;

	i = 0;
	for (;;) {
		/* check if there is room for another pointer (to a group
		   member name, or terminating NULL).  */
		if (i >= size) {
			size = i + 100;	/* at least: i + 1 */
			members = REALLOCF(members, size, char *);
			if (!members) {
				size = 0;
				return NULL;
			}
		}
		if (!s || streq(s, ""))
			break;
		members[i++] = strsep(&s, ",");
	}
	members[i] = NULL;
	return members;
}


struct group *
sgetgrent(const char *s)
{
	static char         *dup = NULL;
	static struct group grent;

	char  *fields[4];

	free(dup);
	dup = strdup(s);
	if (dup == NULL)
		return NULL;

	stpsep(dup, "\n");

	if (STRSEP2ARR(dup, ":", fields) == -1)
		return NULL;

	if (streq(fields[2], ""))
		return NULL;

	grent.gr_name = fields[0];
	grent.gr_passwd = fields[1];
	if (get_gid(fields[2], &grent.gr_gid) == -1) {
		return NULL;
	}
	grent.gr_mem = list(fields[3]);
	if (NULL == grent.gr_mem) {
		return NULL;	/* out of memory */
	}

	return &grent;
}
