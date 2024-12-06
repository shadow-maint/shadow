/*
 * SPDX-FileCopyrightText: 1989 - 1994, Julianne Frances Haugh
 * SPDX-FileCopyrightText: 1996 - 1998, Marek Michałkiewicz
 * SPDX-FileCopyrightText: 2003 - 2005, Tomasz Kłoczko
 * SPDX-FileCopyrightText: 2008       , Nicolas François
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <config.h>

#ident "$Id$"

#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "atoi/getnum.h"
#include "defines.h"
#include "prototypes.h"
#include "shadowlog_internal.h"
#include "string/strcmp/streq.h"
#include "string/strtok/stpsep.h"
#include "string/strtok/strsep2arr.h"


/*
 * sgetpwent - convert a string to a (struct passwd)
 *
 * sgetpwent() parses a string into the parts required for a password
 * structure.  Strict checking is made for the UID and GID fields and
 * presence of the correct number of colons.  Any failing tests result
 * in a NULL pointer being returned.
 *
 * NOTE: This function uses hard-coded string scanning functions for
 *	performance reasons.  I am going to come up with some conditional
 *	compilation glarp to improve on this in the future.
 */
struct passwd *
sgetpwent(const char *s)
{
	static char          *dup = NULL;
	static struct passwd pwent;

	char  *fields[7];

	free(dup);
	dup = strdup(s);
	if (dup == NULL)
		return NULL;

	stpsep(dup, "\n");

	if (STRSEP2ARR(dup, ":", fields) == -1)
		return NULL;

	/*
	 * The UID and GID must be non-blank.
	 */
	if (streq(fields[2], ""))
		return NULL;
	if (streq(fields[3], ""))
		return NULL;

	/*
	 * Each of the fields is converted to the appropriate data type
	 * and the result assigned to the password structure.  If the
	 * UID or GID does not convert to an integer value, a NULL
	 * pointer is returned.
	 */

	pwent.pw_name = fields[0];
	pwent.pw_passwd = fields[1];
	if (get_uid(fields[2], &pwent.pw_uid) == -1) {
		return NULL;
	}
	if (get_gid(fields[3], &pwent.pw_gid) == -1) {
		return NULL;
	}
	pwent.pw_gecos = fields[4];
	pwent.pw_dir = fields[5];
	pwent.pw_shell = fields[6];

	return &pwent;
}

