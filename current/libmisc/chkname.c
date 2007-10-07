/*
 * check_user_name(), check_group_name() - check the new user/group
 * name for validity; return value: 1 - OK, 0 - bad name
 */

#include <config.h>

#include "rcsid.h"
RCSID("$Id: chkname.c,v 1.4 1998/04/16 19:57:43 marekm Exp $")

#include <ctype.h>
#include "defines.h"
#include "chkname.h"

#if HAVE_UTMPX_H
#include <utmpx.h>
#else
#include <utmp.h>
#endif

static int
good_name(const char *name)
{
	/*
	 * User/group names must start with a letter, and may not
	 * contain colons, commas, newlines (used in passwd/group
	 * files...) or any non-printable characters.
	 */
	if (!*name || !isalpha(*name))
		return 0;

	while (*name) {
		if (*name == ':' || *name == ',' ||
		    *name == '\n' || !isprint(*name))
			return 0;

		name++;
	}

	return 1;
}

int
check_user_name(const char *name)
{
#if HAVE_UTMPX_H
	struct utmpx ut;
#else
	struct utmp ut;
#endif

	/*
	 * User names are limited by whatever utmp can
	 * handle (usually max 8 characters).
	 */
	if (strlen(name) > sizeof(ut.ut_user))
		return 0;

	return good_name(name);
}

int
check_group_name(const char *name)
{
	/*
	 * Arbitrary limit for group names - max 16
	 * characters (same as on HP-UX 10).
	 */
	if (strlen(name) > 16)
		return 0;

	return good_name(name);
}
