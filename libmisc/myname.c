/*
 * myname.c - determine the current username and get the passwd entry
 *
 * Copyright (C) 1996 Marek Michalkiewicz <marekm@i17linuxb.ists.pwr.wroc.pl>
 *
 * This code may be freely used, modified and distributed for any purpose.
 * There is no warranty, if it breaks you have to keep both pieces, etc.
 * If you improve it, please send me your changes.  Thanks!
 */

#include <config.h>

#ident "$Id$"

#include "defines.h"
#include <pwd.h>
#include "prototypes.h"
struct passwd *get_my_pwent (void)
{
	struct passwd *pw;
	const char *cp = getlogin ();
	uid_t ruid = getuid ();

	/*
	 * Try getlogin() first - if it fails or returns a non-existent
	 * username, or a username which doesn't match the real UID, fall
	 * back to getpwuid(getuid()).  This should work reasonably with
	 * usernames longer than the utmp limit (8 characters), as well as
	 * shared UIDs - but not both at the same time...
	 *
	 * XXX - when running from su, will return the current user (not
	 * the original user, like getlogin() does).  Does this matter?
	 */
	if (cp && *cp && (pw = xgetpwnam (cp)) && pw->pw_uid == ruid)
		return pw;

	return xgetpwuid (ruid);
}

