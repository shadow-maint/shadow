/*
 * SPDX-FileCopyrightText: 1991 - 1994, Julianne Frances Haugh
 * SPDX-FileCopyrightText: 1996 - 2000, Marek Michałkiewicz
 * SPDX-FileCopyrightText: 2000 - 2006, Tomasz Kłoczko
 * SPDX-FileCopyrightText: 2007 - 2009, Nicolas François
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <config.h>

#ident "$Id$"

#include <stdlib.h>
#include <grp.h>
#include <sys/types.h>

#include "prototypes.h"

/*
 * getgr_nam_gid - Return a pointer to the group specified by a string.
 * The string may be a valid GID or a valid groupname.
 * If the group does not exist on the system, NULL is returned.
 */
extern /*@only@*//*@null@*/struct group *getgr_nam_gid (/*@null@*/const char *grname)
{
	gid_t  gid;

	if (NULL == grname) {
		return NULL;
	}

	if (get_gid(grname, &gid) == -1)
		return xgetgrnam(grname);
	return xgetgrgid(gid);
}

