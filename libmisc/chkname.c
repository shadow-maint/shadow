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
 * check_user_name(), check_group_name() - check the new user/group
 * name for validity; return value: 1 - OK, 0 - bad name
 */

#include <config.h>

#ident "$Id$"

#include <ctype.h>
#include "defines.h"
#include "chkname.h"
#if HAVE_UTMPX_H
#include <utmpx.h>
#else
#include <utmp.h>
#endif
static int good_name (const char *name)
{
	/*
	 * User/group names must match [a-z_][a-z0-9_-]*[$]
	 */
	if (!*name || !((*name >= 'a' && *name <= 'z') || *name == '_'))
		return 0;

	while (*++name) {
		if (!((*name >= 'a' && *name <= 'z') ||
		      (*name >= '0' && *name <= '9') ||
		      *name == '_' || *name == '-' ||
		      (*name == '$' && *(name + 1) == '\0')))
			return 0;
	}

	return 1;
}

int check_user_name (const char *name)
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
	if (strlen (name) > sizeof (ut.ut_user))
		return 0;

	return good_name (name);
}

int check_group_name (const char *name)
{
	/*
	 * Arbitrary limit for group names - max 16
	 * characters (same as on HP-UX 10).
	 */
	if (strlen (name) > 16)
		return 0;

	return good_name (name);
}

