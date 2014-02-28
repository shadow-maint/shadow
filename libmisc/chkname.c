/*
 * Copyright (c) 1990 - 1994, Julianne Frances Haugh
 * Copyright (c) 1996 - 2000, Marek Michałkiewicz
 * Copyright (c) 2001 - 2005, Tomasz Kłoczko
 * Copyright (c) 2005 - 2008, Nicolas François
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
 * is_valid_user_name(), is_valid_group_name() - check the new user/group
 * name for validity;
 * return values:
 *   true  - OK
 *   false - bad name
 */

#include <config.h>

#ident "$Id$"

#include <string.h>
#include <ctype.h>
#include "defines.h"
#include "chkname.h"

static bool is_valid_name (const char *name)
{
	/*
	 * User/group names must match [a-z_][a-z0-9_-]*[$]
	 */
	if (('\0' == *name) ||
	    !((('a' <= *name) && ('z' >= *name)) || ('_' == *name))) {
		return false;
	}
	/*
 *  The following usernames have been retired.
 *
 *  dmr - Dennis MacAlistair Ritchie (September 9, 1941 - October 12, 2011)
 *  mrc - Mark Reed Crispin (July 19, 1956 - December 28, 2012)
 *  jmc - John McCarthy (September 4, 1927 – October 24, 2011)
 *
 */

	if (strcmp(name, "dmr") ||
	    strcmp(name, "mcr") ||
	    strcmp(name, "jmc")) {
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

