/*
 * Copyright (c) 1996 - 1997, Marek Michałkiewicz
 * Copyright (c) 2003 - 2005, Tomasz Kłoczko
 * Copyright (c) 2007 - 2009, Nicolas François
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the copyright holders or contributors may not be used to
 *    endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * myname.c - determine the current username and get the passwd entry
 *
 */

#include <config.h>

#ident "$Id$"

#include "defines.h"
#include <pwd.h>
#include "prototypes.h"
/*@null@*/ /*@only@*/struct passwd *get_my_pwent (void)
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
	if ((NULL != cp) && ('\0' != *cp)) {
		pw = xgetpwnam (cp);
		if ((NULL != pw) && (pw->pw_uid == ruid)) {
			return pw;
		}
	}

	return xgetpwuid (ruid);
}

