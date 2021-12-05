/*
 * SPDX-FileCopyrightText: 1989 - 1994, Julianne Frances Haugh
 * SPDX-FileCopyrightText: 1996 - 1998, Marek Michałkiewicz
 * SPDX-FileCopyrightText: 2003 - 2005, Tomasz Kłoczko
 * SPDX-FileCopyrightText: 2009       , Nicolas François
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <config.h>

/* Newer versions of Linux libc already have shadow support.  */
#ifndef HAVE_SGETSPENT

#ident "$Id$"

#include <sys/types.h>
#include "prototypes.h"
#include "defines.h"
#include <stdio.h>
#define	FIELDS	9
#define	OFIELDS	5
/*
 * sgetspent - convert string in shadow file format to (struct spwd *)
 */
struct spwd *sgetspent (const char *string)
{
	static char spwbuf[1024];
	static struct spwd spwd;
	char *fields[FIELDS];
	char *cp;
	int i;

	/*
	 * Copy string to local buffer.  It has to be tokenized and we
	 * have to do that to our private copy.
	 */

	if (strlen (string) >= sizeof spwbuf) {
		return 0;	/* fail if too long */
	}
	strcpy (spwbuf, string);

	cp = strrchr (spwbuf, '\n');
	if (NULL != cp) {
		*cp = '\0';
	}

	/*
	 * Tokenize the string into colon separated fields.  Allow up to
	 * FIELDS different fields.
	 */

	for (cp = spwbuf, i = 0; ('\0' != *cp) && (i < FIELDS); i++) {
		fields[i] = cp;
		while (('\0' != *cp) && (':' != *cp)) {
			cp++;
		}

		if ('\0' != *cp) {
			*cp = '\0';
			cp++;
		}
	}

	if (i == (FIELDS - 1)) {
		fields[i++] = cp;
	}

	if ( ((NULL != cp) && ('\0' != *cp)) ||
	     ((i != FIELDS) && (i != OFIELDS)) ) {
		return 0;
	}

	/*
	 * Start populating the structure.  The fields are all in
	 * static storage, as is the structure we pass back.
	 */

	spwd.sp_namp = fields[0];
	spwd.sp_pwdp = fields[1];

	/*
	 * Get the last changed date.  For all of the integer fields,
	 * we check for proper format.  It is an error to have an
	 * incorrectly formatted number.
	 */

	if (fields[2][0] == '\0') {
		spwd.sp_lstchg = -1;
	} else if (   (getlong (fields[2], &spwd.sp_lstchg) == 0)
	           || (spwd.sp_lstchg < 0)) {
		return 0;
	}

	/*
	 * Get the minimum period between password changes.
	 */

	if (fields[3][0] == '\0') {
		spwd.sp_min = -1;
	} else if (   (getlong (fields[3], &spwd.sp_min) == 0)
	           || (spwd.sp_min < 0)) {
		return 0;
	}

	/*
	 * Get the maximum number of days a password is valid.
	 */

	if (fields[4][0] == '\0') {
		spwd.sp_max = -1;
	} else if (   (getlong (fields[4], &spwd.sp_max) == 0)
	           || (spwd.sp_max < 0)) {
		return 0;
	}

	/*
	 * If there are only OFIELDS fields (this is a SVR3.2 /etc/shadow
	 * formatted file), initialize the other field members to -1.
	 */

	if (i == OFIELDS) {
		spwd.sp_warn   = -1;
		spwd.sp_inact  = -1;
		spwd.sp_expire = -1;
		spwd.sp_flag   = SHADOW_SP_FLAG_UNSET;

		return &spwd;
	}

	/*
	 * Get the number of days of password expiry warning.
	 */

	if (fields[5][0] == '\0') {
		spwd.sp_warn = -1;
	} else if (   (getlong (fields[5], &spwd.sp_warn) == 0)
	           || (spwd.sp_warn < 0)) {
		return 0;
	}

	/*
	 * Get the number of days of inactivity before an account is
	 * disabled.
	 */

	if (fields[6][0] == '\0') {
		spwd.sp_inact = -1;
	} else if (   (getlong (fields[6], &spwd.sp_inact) == 0)
	           || (spwd.sp_inact < 0)) {
		return 0;
	}

	/*
	 * Get the number of days after the epoch before the account is
	 * set to expire.
	 */

	if (fields[7][0] == '\0') {
		spwd.sp_expire = -1;
	} else if (   (getlong (fields[7], &spwd.sp_expire) == 0)
	           || (spwd.sp_expire < 0)) {
		return 0;
	}

	/*
	 * This field is reserved for future use.  But it isn't supposed
	 * to have anything other than a valid integer in it.
	 */

	if (fields[8][0] == '\0') {
		spwd.sp_flag = SHADOW_SP_FLAG_UNSET;
	} else if (getlong (fields[8], &spwd.sp_flag) == 0) {
		/* FIXME: add a getulong function */
		return 0;
	}

	return (&spwd);
}
#else
extern int errno;		/* warning: ANSI C forbids an empty source file */
#endif

