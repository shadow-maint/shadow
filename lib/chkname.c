// SPDX-FileCopyrightText: 1990-1994, Julianne Frances Haugh
// SPDX-FileCopyrightText: 1996-2000, Marek Michałkiewicz
// SPDX-FileCopyrightText: 2001-2005, Tomasz Kłoczko
// SPDX-FileCopyrightText: 2005-2008, Nicolas François
// SPDX-FileCopyrightText: 2023-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


/*
 * is_valid_user_name(), is_valid_group_name() - check the new user/group
 * name for validity;
 * return values:
 *   true  - OK
 *   false - bad name
 * errors:
 *   EINVAL	Invalid name
 *   EILSEQ	Invalid name character sequence (acceptable with --badname)
 *   EOVERFLOW	Name longer than maximum size
 */


#include "config.h"

#ident "$Id$"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>

#include "defines.h"
#include "chkname.h"
#include "string/ctype/strchrisascii.h"
#include "string/ctype/strisascii.h"
#include "string/strcmp/streq.h"
#include "string/strcmp/strcaseeq.h"


#ifndef  LOGIN_NAME_MAX
# define LOGIN_NAME_MAX  256
#endif


int allow_bad_names = false;


size_t
login_name_max_size(void)
{
	long  conf;

	conf = sysconf(_SC_LOGIN_NAME_MAX);
	if (conf == -1)
		return LOGIN_NAME_MAX;

	return conf;
}


static bool
is_valid_name(const char *name)
{
	if (streq(name, "")
	 || streq(name, ".")
	 || streq(name, "..")
	 || strspn(name, "-")
	 || strpbrk(name, " \"#',/:;")
	 || strchriscntrl_c(name)
	 || strisdigit_c(name))
	{
		errno = EINVAL;
		return false;
	}

	if (allow_bad_names) {
		return true;
	}

	/*
	 * User/group names must match BRE regex:
	 *    [a-zA-Z0-9_.][a-zA-Z0-9_.-]*$\?
	 *
	 * as a non-POSIX, extension, allow "$" as the last char for
	 * sake of Samba 3.x "add machine script"
	 */

	if (!(isalnum(*name) ||
	      *name == '_' ||
	      *name == '.'))
	{
		errno = EILSEQ;
		return false;
	}

	while (!streq(++name, "")) {
		if (streq(name, "$"))  // Samba
			return true;

		if (!(isalnum(*name) ||
		      *name == '_' ||
		      *name == '.' ||
		      *name == '-'
		     ))
		{
			errno = EILSEQ;
			return false;
		}
	}

	return true;
}


bool
is_valid_user_name(const char *name)
{
	if (strlen(name) >= login_name_max_size()) {
		errno = EOVERFLOW;
		return false;
	}

	return is_valid_name(name);
}


bool
is_valid_group_name(const char *name)
{
	/*
	 * Arbitrary limit for group names.
	 * HP-UX 10 limits to 16 characters
	 */
	if (   (GROUP_NAME_MAX_LENGTH > 0)
	    && (strlen (name) > GROUP_NAME_MAX_LENGTH))
	{
		errno = EOVERFLOW;
		return false;
	}

	return is_valid_name (name);
}
