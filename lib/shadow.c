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
#ifndef HAVE_GETSPNAM

#ident "$Id$"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include "atoi/a2i/a2s.h"
#include "atoi/str2i/str2u.h"
#include "defines.h"
#include "prototypes.h"
#include "string/strcmp/streq.h"
#include "string/strtok/stpsep.h"


static FILE *shadow;

#define	FIELDS	9
#define	OFIELDS	5


/*
 * setspent - initialize access to shadow text and DBM files
 */

void setspent (void)
{
	if (NULL != shadow) {
		rewind (shadow);
	}else {
		shadow = fopen (SHADOW_FILE, "re");
	}
}

/*
 * endspent - terminate access to shadow text and DBM files
 */

void endspent (void)
{
	if (NULL != shadow) {
		(void) fclose (shadow);
	}

	shadow = NULL;
}

/*
 * fgetspent - get an entry from a /etc/shadow formatted stream
 */

struct spwd *fgetspent (FILE * fp)
{
	char  buf[BUFSIZ];

	if (NULL == fp) {
		return (0);
	}

	if (fgets (buf, sizeof buf, fp) != NULL)
	{
		stpsep(buf, "\n");
		return sgetspent(buf);
	}
	return 0;
}

/*
 * getspent - get a (struct spwd *) from the current shadow file
 */

struct spwd *getspent (void)
{
	if (NULL == shadow) {
		setspent ();
	}
	return (fgetspent (shadow));
}

/*
 * getspnam - get a shadow entry by name
 */

struct spwd *getspnam (const char *name)
{
	struct spwd *sp;

	setspent ();

	while ((sp = getspent ()) != NULL) {
		if (streq(name, sp->sp_namp)) {
			break;
		}
	}
	endspent ();
	return (sp);
}
#else
extern int ISO_C_forbids_an_empty_translation_unit;
#endif
