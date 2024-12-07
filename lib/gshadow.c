/*
 * SPDX-FileCopyrightText: 1990 - 1994, Julianne Frances Haugh
 * SPDX-FileCopyrightText: 1996 - 1998, Marek Michałkiewicz
 * SPDX-FileCopyrightText: 2005       , Tomasz Kłoczko
 * SPDX-FileCopyrightText: 2008 - 2009, Nicolas François
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <config.h>

#if defined(SHADOWGRP) && !__has_include(<gshadow.h>)

#ident "$Id$"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "alloc/malloc.h"
#include "alloc/realloc.h"
#include "defines.h"
#include "prototypes.h"
#include "string/strcmp/streq.h"
#include "string/strtok/stpsep.h"
#include "string/strtok/strsep2arr.h"
#include "string/strtok/xastrsep2ls.h"


static /*@null@*/FILE *shadow;
static struct sgrp  sgroup = {};


static /*@null@*/char **
build_list(char *s)
{
	char    **l;
	size_t  n;

	l = xastrsep2ls(s, ",", &n);

	if (streq(l[n-1], ""))
		l[n-1] = NULL;

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
sgetsgent(const char *s)
{
	static char  *dup = NULL;

	char  *fields[4];

	free(dup);
	dup = strdup(s);
	if (dup == NULL)
		return NULL;

	stpsep(dup, "\n");

	if (STRSEP2ARR(dup, ":", fields) == -1)
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
	size = strlen (sgrp->sg_namp) + strlen (sgrp->sg_passwd) + 10;
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
	cp = stpcpy(stpcpy(cp, sgrp->sg_namp), ":");
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
#endif  // !SHADOWGRP
