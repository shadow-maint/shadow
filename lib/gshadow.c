/*
 * SPDX-FileCopyrightText: 1990 - 1994, Julianne Frances Haugh
 * SPDX-FileCopyrightText: 1996 - 1998, Marek Michałkiewicz
 * SPDX-FileCopyrightText: 2005       , Tomasz Kłoczko
 * SPDX-FileCopyrightText: 2008 - 2009, Nicolas François
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <config.h>

#if defined(SHADOWGRP) && !defined(HAVE_GSHADOW_H)

#ident "$Id$"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "alloc/malloc.h"
#include "alloc/realloc.h"
#include "alloc/x/xmalloc.h"
#include "defines.h"
#include "prototypes.h"
#include "string/strchr/strchrcnt.h"
#include "string/strcmp/streq.h"
#include "string/strtok/stpsep.h"


static /*@null@*/FILE *shadow;
static struct sgrp  sgroup = {};

#define	FIELDS	4


static /*@null@*/char **
build_list(char *s)
{
	char    **l;
	size_t  i;

	l = XMALLOC(strchrcnt(s, ',') + 2, char *);

	for (i = 0; s != NULL && !streq(s, ""); i++)
		l[i] = strsep(&s, ",");

	l[i] = NULL;

	return l;
}

void setsgent (void)
{
	if (NULL != shadow) {
		rewind (shadow);
	} else {
		shadow = fopen (SGROUP_FILE, "re");
	}
}

void endsgent (void)
{
	if (NULL != shadow) {
		(void) fclose (shadow);
	}

	shadow = NULL;
}

/*@observer@*//*@null@*/struct sgrp *
sgetsgent(const char *string)
{
	static char *sgrbuf = NULL;
	static size_t sgrbuflen = 0;

	char *fields[FIELDS];
	char *cp;
	int i;
	size_t len = strlen (string) + 1;

	if (len > sgrbuflen) {
		char *buf = REALLOC(sgrbuf, len, char);
		if (NULL == buf)
			return NULL;

		sgrbuf = buf;
		sgrbuflen = len;
	}

	strcpy (sgrbuf, string);
	stpsep(sgrbuf, "\n");

	/*
	 * There should be exactly 4 colon separated fields.  Find
	 * all 4 of them and save the starting addresses in fields[].
	 */

	for (cp = sgrbuf, i = 0; (i < FIELDS) && (NULL != cp); i++)
		fields[i] = strsep(&cp, ":");

	/*
	 * If there was an extra field somehow, or perhaps not enough,
	 * the line is invalid.
	 */

	if (NULL != cp || i != FIELDS)
		return NULL;

	sgroup.sg_namp = fields[0];
	sgroup.sg_passwd = fields[1];

	free(sgroup.sg_adm);
	free(sgroup.sg_mem);

	sgroup.sg_adm = build_list(fields[2]);
	sgroup.sg_mem = build_list(fields[3]);

	return &sgroup;
}

/*
 * fgetsgent - convert next line in stream to (struct sgrp)
 *
 * fgetsgent() reads the next line from the provided stream and
 * converts it to a (struct sgrp).  NULL is returned on EOF.
 */

/*@observer@*//*@null@*/struct sgrp *fgetsgent (/*@null@*/FILE * fp)
{
	static size_t buflen = 0;
	static char *buf = NULL;

	char *cp;

	if (0 == buflen) {
		buf = MALLOC(BUFSIZ, char);
		if (NULL == buf) {
			return NULL;
		}
		buflen = BUFSIZ;
	}

	if (NULL == fp) {
		return NULL;
	}

	if (fgetsx(buf, buflen, fp) == NULL)
		return NULL;

	while (   (strrchr(buf, '\n') == NULL)
	       && (feof (fp) == 0)) {
		size_t len;

		cp = REALLOC(buf, buflen * 2, char);
		if (NULL == cp) {
			return NULL;
		}
		buf = cp;
		buflen *= 2;

		len = strlen (buf);
		if (fgetsx (&buf[len],
			    (int) (buflen - len),
			    fp) != &buf[len]) {
			return NULL;
		}
	}
	stpsep(buf, "\n");
	return (sgetsgent (buf));
}

/*
 * getsgent - get a single shadow group entry
 */

/*@observer@*//*@null@*/struct sgrp *getsgent (void)
{
	if (NULL == shadow) {
		setsgent ();
	}
	return (fgetsgent (shadow));
}

/*
 * getsgnam - get a shadow group entry by name
 */

/*@observer@*//*@null@*/struct sgrp *getsgnam (const char *name)
{
	struct sgrp *sgrp;

	setsgent ();

	while ((sgrp = getsgent ()) != NULL) {
		if (streq(name, sgrp->sg_namp)) {
			break;
		}
	}
	return sgrp;
}
#else
extern int ISO_C_forbids_an_empty_translation_unit;
#endif  // !SHADOWGRP
