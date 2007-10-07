/*
 * Copyright 1990 - 1994, Julianne Frances Haugh
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
 * 3. Neither the name of Julianne F. Haugh nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY JULIE HAUGH AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL JULIE HAUGH OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <config.h>

/* Newer versions of Linux libc already have shadow support.  */
#if defined(SHADOWGRP) && !defined(HAVE_SHADOWGRP)	/*{ */

#include "rcsid.h"
RCSID ("$Id: gshadow.c,v 1.10 2005/05/25 19:31:50 kloczek Exp $")
#include <stdio.h>
#include "prototypes.h"
#include "defines.h"
static FILE *shadow;
static char sgrbuf[BUFSIZ * 4];
static char **members = NULL;
static size_t nmembers = 0;
static char **admins = NULL;
static size_t nadmins = 0;
static struct sgrp sgroup;

extern char *fgetsx ();
extern int fputsx ();

#define	FIELDS	4

#ifdef	USE_NIS
static int nis_used;
static int nis_ignore;
static enum { native, start, middle, native2 } nis_state;
static int nis_bound;
static char *nis_domain;
static char *nis_key;
static int nis_keylen;
static char *nis_val;
static int nis_vallen;

#define	IS_NISCHAR(c) ((c)=='+')
#endif

#ifdef	USE_NIS

/*
 * __setsgNIS - turn on or off NIS searches
 */

void __setsgNIS (int flag)
{
	nis_ignore = !flag;

	if (nis_ignore)
		nis_used = 0;
}

/*
 * bind_nis - bind to NIS server
 */

static int bind_nis (void)
{
	if (yp_get_default_domain (&nis_domain))
		return -1;

	nis_bound = 1;
	return 0;
}
#endif

static char **list (char *s, char **list[], size_t * nlist)
{
	char **ptr = *list;
	size_t nelem = *nlist, size;

	while (s != NULL && *s != '\0') {
		size = (nelem + 1) * sizeof (ptr);
		if ((ptr = realloc (*list, size)) != NULL) {
			ptr[nelem++] = s;
			*list = ptr;
			*nlist = nelem;
			if ((s = strchr (s, ',')))
				*s++ = '\0';
		}
	}
	size = (nelem + 1) * sizeof (ptr);
	if ((ptr = realloc (*list, size)) != NULL) {
		ptr[nelem] = '\0';
		*list = ptr;
	}
	return ptr;
}

void setsgent (void)
{
#ifdef	USE_NIS
	nis_state = native;
#endif
	if (shadow)
		rewind (shadow);
	else
		shadow = fopen (SGROUP_FILE, "r");
}

void endsgent (void)
{
	if (shadow)
		(void) fclose (shadow);

	shadow = (FILE *) 0;
}

struct sgrp *sgetsgent (const char *string)
{
	char *fields[FIELDS];
	char *cp;
	int i;

	strncpy (sgrbuf, string, (int) sizeof sgrbuf - 1);
	sgrbuf[sizeof sgrbuf - 1] = '\0';

	if ((cp = strrchr (sgrbuf, '\n')))
		*cp = '\0';

	/*
	 * There should be exactly 4 colon separated fields.  Find
	 * all 4 of them and save the starting addresses in fields[].
	 */

	for (cp = sgrbuf, i = 0; i < FIELDS && cp; i++) {
		fields[i] = cp;
		if ((cp = strchr (cp, ':')))
			*cp++ = '\0';
	}

	/*
	 * If there was an extra field somehow, or perhaps not enough,
	 * the line is invalid.
	 */

	if (cp || i != FIELDS)
#ifdef	USE_NIS
		if (!IS_NISCHAR (fields[0][0]))
			return 0;
		else
			nis_used = 1;
#else
		return 0;
#endif

	sgroup.sg_name = fields[0];
	sgroup.sg_passwd = fields[1];
	if (nadmins) {
		nadmins = 0;
		free (admins);
		admins = NULL;
	}
	if (nmembers) {
		nmembers = 0;
		free (members);
		members = NULL;
	}
	sgroup.sg_adm = list (fields[2], &admins, &nadmins);
	sgroup.sg_mem = list (fields[3], &members, &nmembers);

	return &sgroup;
}

/*
 * fgetsgent - convert next line in stream to (struct sgrp)
 *
 * fgetsgent() reads the next line from the provided stream and
 * converts it to a (struct sgrp).  NULL is returned on EOF.
 */

struct sgrp *fgetsgent (FILE * fp)
{
	char buf[sizeof sgrbuf];
	char *cp;

	if (!fp)
		return (0);

#ifdef	USE_NIS
	while (fgetsx (buf, sizeof buf, fp) != (char *) 0)
#else
	if (fgetsx (buf, sizeof buf, fp) != (char *) 0)
#endif
	{
		if ((cp = strchr (buf, '\n')))
			*cp = '\0';
#ifdef	USE_NIS
		if (nis_ignore && IS_NISCHAR (buf[0]))
			continue;
#endif
		return (sgetsgent (buf));
	}
	return 0;
}

/*
 * getsgent - get a single shadow group entry
 */

struct sgrp *getsgent (void)
{
#ifdef	USE_NIS
	int nis_1_group = 0;
	struct sgrp *val;
	char buf[BUFSIZ];
#endif
	if (!shadow)
		setsgent ();

#ifdef	USE_NIS
      again:
	/*
	 * See if we are reading from the local file.
	 */

	if (nis_state == native || nis_state == native2) {

		/*
		 * Get the next entry from the shadow group file.  Return
		 * NULL right away if there is none.
		 */

		if (!(val = fgetsgent (shadow)))
			return 0;

		/*
		 * If this entry began with a NIS escape character, we have
		 * to see if this is just a single group, or if the entire
		 * map is being asked for.
		 */

		if (IS_NISCHAR (val->sg_name[0])) {
			if (val->sg_name[1])
				nis_1_group = 1;
			else
				nis_state = start;
		}

		/*
		 * If this isn't a NIS group and this isn't an escape to go
		 * use a NIS map, it must be a regular local group.
		 */

		if (nis_1_group == 0 && nis_state != start)
			return val;

		/*
		 * If this is an escape to use an NIS map, switch over to
		 * that bunch of code.
		 */

		if (nis_state == start)
			goto again;

		/*
		 * NEEDSWORK.  Here we substitute pieces-parts of this entry.
		 */

		return 0;
	} else {
		if (nis_bound == 0) {
			if (bind_nis ()) {
				nis_state = native2;
				goto again;
			}
		}
		if (nis_state == start) {
			if (yp_first (nis_domain, "gshadow.byname", &nis_key,
				      &nis_keylen, &nis_val, &nis_vallen)) {
				nis_state = native2;
				goto again;
			}
			nis_state = middle;
		} else if (nis_state == middle) {
			if (yp_next (nis_domain, "gshadow.byname", nis_key,
				     nis_keylen, &nis_key, &nis_keylen,
				     &nis_val, &nis_vallen)) {
				nis_state = native2;
				goto again;
			}
		}
		return sgetsgent (nis_val);
	}
#else
	return (fgetsgent (shadow));
#endif
}

/*
 * getsgnam - get a shadow group entry by name
 */

struct sgrp *getsgnam (const char *name)
{
	struct sgrp *sgrp;

#ifdef	USE_NIS
	char buf[BUFSIZ];
	static char save_name[16];
	int nis_disabled = 0;
#endif

	setsgent ();

#ifdef	USE_NIS
	if (nis_used) {
	      again:

		/*
		 * Search the gshadow.byname map for this group.
		 */

		if (!nis_bound)
			bind_nis ();

		if (nis_bound) {
			char *cp;

			if (yp_match (nis_domain, "gshadow.byname", name,
				      strlen (name), &nis_val,
				      &nis_vallen) == 0) {
				if (cp = strchr (nis_val, '\n'))
					*cp = '\0';

				nis_state = middle;
				if (sgrp = sgetsgent (nis_val)) {
					strcpy (save_name, sgrp->sg_name);
					nis_key = save_name;
					nis_keylen = strlen (save_name);
				}
				return sgrp;
			}
		}
		nis_state = native2;
	}
#endif
#ifdef	USE_NIS
	if (nis_used) {
		nis_ignore++;
		nis_disabled++;
	}
#endif
	while ((sgrp = getsgent ()) != (struct sgrp *) 0) {
		if (strcmp (name, sgrp->sg_name) == 0)
			break;
	}
#ifdef	USE_NIS
	nis_ignore--;
#endif
	if (sgrp)
		return sgrp;
	return (0);
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

	if (!fp || !sgrp)
		return -1;

	/* calculate the required buffer size */
	size = strlen (sgrp->sg_name) + strlen (sgrp->sg_passwd) + 10;
	for (i = 0; sgrp->sg_adm && sgrp->sg_adm[i]; i++)
		size += strlen (sgrp->sg_adm[i]) + 1;
	for (i = 0; sgrp->sg_mem && sgrp->sg_mem[i]; i++)
		size += strlen (sgrp->sg_mem[i]) + 1;

	buf = malloc (size);
	if (!buf)
		return -1;
	cp = buf;

	/*
	 * Copy the group name and passwd.
	 */

	strcpy (cp, sgrp->sg_name);
	cp += strlen (cp);
	*cp++ = ':';

	strcpy (cp, sgrp->sg_passwd);
	cp += strlen (cp);
	*cp++ = ':';

	/*
	 * Copy the administrators, separating each from the other
	 * with a ",".
	 */

	for (i = 0; sgrp->sg_adm[i]; i++) {
		if (i > 0)
			*cp++ = ',';

		strcpy (cp, sgrp->sg_adm[i]);
		cp += strlen (cp);
	}
	*cp++ = ':';

	/*
	 * Now do likewise with the group members.
	 */

	for (i = 0; sgrp->sg_mem[i]; i++) {
		if (i > 0)
			*cp++ = ',';

		strcpy (cp, sgrp->sg_mem[i]);
		cp += strlen (cp);
	}
	*cp++ = '\n';
	*cp = '\0';

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
extern int errno;		/* warning: ANSI C forbids an empty source file */
#endif				/*} SHADOWGRP */
