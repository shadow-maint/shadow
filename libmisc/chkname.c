/*
 * SPDX-FileCopyrightText: 1990 - 1994, Julianne Frances Haugh
 * SPDX-FileCopyrightText: 1996 - 2000, Marek Michałkiewicz
 * SPDX-FileCopyrightText: 2001 - 2005, Tomasz Kłoczko
 * SPDX-FileCopyrightText: 2005 - 2008, Nicolas François
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * is_valid_user_name(), is_valid_group_name() - check the new user/group
 * name for validity;
 * return values:
 *   true  - OK
 *   false - bad name
 */

#include <config.h>

#ident "$Id$"

#include <ctype.h>
#include "defines.h"
#include "chkname.h"

int allow_bad_names = false;

static bool is_valid_name (const char *name)
{
	if (allow_bad_names) {
		return true;
	}

	/*
	 * User/group names must match [a-z_][a-z0-9_-]*[$]
	 */

	if (('\0' == *name) ||
	    !((('a' <= *name) && ('z' >= *name)) || ('_' == *name))) {
		return false;
	}

	while ('\0' != *++name) {
		if (!(( ('a' <= *name) && ('z' >= *name) ) ||
		      ( ('0' <= *name) && ('9' >= *name) ) ||
		      ('_' == *name) ||
		      ('-' == *name) ||
		      ( ('$' == *name) && ('\0' == *(name + 1)) )
		     )) {
			return false;
		}
	}

	return true;
}

bool is_valid_user_name (const char *name)
{
	/*
	 * User names are limited by whatever utmp can
	 * handle.
	 */
	if (strlen (name) > USER_NAME_MAX_LENGTH) {
		return false;
	}

	return is_valid_name (name);
}

bool is_valid_group_name (const char *name)
{
	/*
	 * Arbitrary limit for group names.
	 * HP-UX 10 limits to 16 characters
	 */
	if (   (GROUP_NAME_MAX_LENGTH > 0)
	    && (strlen (name) > GROUP_NAME_MAX_LENGTH)) {
		return false;
	}

	return is_valid_name (name);
}

