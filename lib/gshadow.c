/*
 * SPDX-FileCopyrightText: 1990 - 1994, Julianne Frances Haugh
 * SPDX-FileCopyrightText: 1996 - 1998, Marek Michałkiewicz
 * SPDX-FileCopyrightText: 2005       , Tomasz Kłoczko
 * SPDX-FileCopyrightText: 2008 - 2009, Nicolas François
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <config.h>

/* Newer versions of Linux libc already have shadow support.  */
#if defined(SHADOWGRP) && !defined(HAVE_SHADOWGRP)	/*{ */

#ident "$Id$"

#include <stdio.h>
#include <string.h>

#include "alloc/malloc.h"
#include "alloc/realloc.h"
#include "alloc/x/xrealloc.h"
#include "defines.h"
#include "prototypes.h"
#include "string/strcmp/streq.h"
#include "string/strtok/stpsep.h"


static /*@null@*/FILE *shadow;
static /*@null@*//*@only@*/char **members = NULL;
static size_t nmembers = 0;
static /*@null@*//*@only@*/char **admins = NULL;
static size_t nadmins = 0;
static struct sgrp sgroup;

#define	FIELDS	4


static /*@null@*/char **
build_list(char *s, char ***lp, size_t *np)
{
	char    **l;
	size_t  n = *np;

	while (s != NULL && *s != '\0') {
		l = XREALLOC(*lp, n + 1, char *);
		l[n] = strsep(&s, ",");
		n++;
		*lp = l;
		*np = n;
	}

	l = XREALLOC(*lp, n + 1, char *);
	l[n] = NULL;
	*lp = l;

	return l;
}

void setsgent (void)
{
	if (NULL != shadow) {
		rewind (shadow);
	} else {
		shadow = fopen (SGROUP_FILE, "r");
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
		if (NULL == buf) {
			return NULL;
		}
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
		return 0;

	sgroup.sg_name = fields[0];
	sgroup.sg_passwd = fields[1];

	nadmins = 0;
	free (admins);
	admins = NULL;
	nmembers = 0;
	free (members);
	members = NULL;

	sgroup.sg_adm = build_list (fields[2], &admins, &nadmins);
	sgroup.sg_mem = build_list (fields[3], &members, &nmembers);

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
		if (streq(name, sgrp->sg_name)) {
			break;
		}
	}
	return sgrp;
}

/*
 * putsgent - output shadow group entry in text form
 *
 * putsgent() converts the contents of a (struct sgrp) to text and
 * writes the result to the given stream.  This is the logical
 * opposite of fgetsgent.
 */

int putsgent (const struct sgrp *sgrp, FILE * fp)
{
	char *buf, *cp;
	int i;
	size_t size;

	if ((NULL == fp) || (NULL == sgrp)) {
		return -1;
	}

	/* calculate the required buffer size */
	size = strlen (sgrp->sg_name) + strlen (sgrp->sg_passwd) + 10;
	for (i = 0; (NULL != sgrp->sg_adm) && (NULL != sgrp->sg_adm[i]); i++) {
		size += strlen (sgrp->sg_adm[i]) + 1;
	}
	for (i = 0; (NULL != sgrp->sg_mem) && (NULL != sgrp->sg_mem[i]); i++) {
		size += strlen (sgrp->sg_mem[i]) + 1;
	}

	buf = MALLOC(size, char);
	if (NULL == buf) {
		return -1;
	}
	cp = buf;

	/*
	 * Copy the group name and passwd.
	 */
	cp = stpcpy(stpcpy(cp, sgrp->sg_name), ":");
	cp = stpcpy(stpcpy(cp, sgrp->sg_passwd), ":");

	/*
	 * Copy the administrators, separating each from the other
	 * with a ",".
	 */
	for (i = 0; NULL != sgrp->sg_adm[i]; i++) {
		if (i > 0)
			cp = stpcpy(cp, ",");

		cp = stpcpy(cp, sgrp->sg_adm[i]);
	}
	cp = stpcpy(cp, ":");

	/*
	 * Now do likewise with the group members.
	 */
	for (i = 0; NULL != sgrp->sg_mem[i]; i++) {
		if (i > 0)
			cp = stpcpy(cp, ",");

		cp = stpcpy(cp, sgrp->sg_mem[i]);
	}
	stpcpy(cp, "\n");

	/*
	 * Output using the function which understands the line
	 * continuation conventions.
	 */
	if (fputsx (buf, fp) == EOF) {
		free (buf);
		return -1;
	}

	free (buf);
	return 0;
}
#else
extern int ISO_C_forbids_an_empty_translation_unit;
#endif				/*} SHADOWGRP */
