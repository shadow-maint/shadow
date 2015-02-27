/*
 * Copyright (c) 1990 - 1994, Julianne Frances Haugh
 * Copyright (c) 1996 - 2000, Marek Michałkiewicz
 * Copyright (c) 2001       , Michał Moskal
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

#include <config.h>

#ident "$Id$"

#include "prototypes.h"
#include "defines.h"
#include <pwd.h>
#include <stdio.h>
#include "commonio.h"
#include "pwio.h"

static /*@null@*/ /*@only@*/void *passwd_dup (const void *ent)
{
	const struct passwd *pw = ent;

	return __pw_dup (pw);
}

static void passwd_free (/*@out@*/ /*@only@*/void *ent)
{
	struct passwd *pw = ent;

	pw_free (pw);
}

static const char *passwd_getname (const void *ent)
{
	const struct passwd *pw = ent;

	return pw->pw_name;
}

static void *passwd_parse (const char *line)
{
	return (void *) sgetpwent (line);
}

static int passwd_put (const void *ent, FILE * file)
{
	const struct passwd *pw = ent;

	if (   (NULL == pw)
	    || (valid_field (pw->pw_name, ":\n") == -1)
	    || (valid_field (pw->pw_passwd, ":\n") == -1)
	    || (pw->pw_uid == (uid_t)-1)
	    || (pw->pw_gid == (gid_t)-1)
	    || (valid_field (pw->pw_gecos, ":\n") == -1)
	    || (valid_field (pw->pw_dir, ":\n") == -1)
	    || (valid_field (pw->pw_shell, ":\n") == -1)) {
		return -1;
	}

	return (putpwent (pw, file) == -1) ? -1 : 0;
}

static struct commonio_ops passwd_ops = {
	passwd_dup,
	passwd_free,
	passwd_getname,
	passwd_parse,
	passwd_put,
	fgets,
	fputs,
	NULL,			/* open_hook */
	NULL			/* close_hook */
};

static struct commonio_db passwd_db = {
	PASSWD_FILE,		/* filename */
	&passwd_ops,		/* ops */
	NULL,			/* fp */
#ifdef WITH_SELINUX
	NULL,			/* scontext */
#endif
	0644,                   /* st_mode */
	0,                      /* st_uid */
	0,                      /* st_gid */
	NULL,			/* head */
	NULL,			/* tail */
	NULL,			/* cursor */
	false,			/* changed */
	false,			/* isopen */
	false,			/* locked */
	false			/* readonly */
};

int pw_setdbname (const char *filename)
{
	return commonio_setname (&passwd_db, filename);
}

/*@observer@*/const char *pw_dbname (void)
{
	return passwd_db.filename;
}

int pw_lock (void)
{
	return commonio_lock (&passwd_db);
}

int pw_open (int mode)
{
	return commonio_open (&passwd_db, mode);
}

/*@observer@*/ /*@null@*/const struct passwd *pw_locate (const char *name)
{
	return commonio_locate (&passwd_db, name);
}

/*@observer@*/ /*@null@*/const struct passwd *pw_locate_uid (uid_t uid)
{
	const struct passwd *pwd;

	pw_rewind ();
	while (   ((pwd = pw_next ()) != NULL)
	       && (pwd->pw_uid != uid)) {
	}

	return pwd;
}

int pw_update (const struct passwd *pw)
{
	return commonio_update (&passwd_db, (const void *) pw);
}

int pw_remove (const char *name)
{
	return commonio_remove (&passwd_db, name);
}

int pw_rewind (void)
{
	return commonio_rewind (&passwd_db);
}

/*@observer@*/ /*@null@*/const struct passwd *pw_next (void)
{
	return commonio_next (&passwd_db);
}

int pw_close (void)
{
	return commonio_close (&passwd_db);
}

int pw_unlock (void)
{
	return commonio_unlock (&passwd_db);
}

/*@null@*/struct commonio_entry *__pw_get_head (void)
{
	return passwd_db.head;
}

void __pw_del_entry (const struct commonio_entry *ent)
{
	commonio_del_entry (&passwd_db, ent);
}

struct commonio_db *__pw_get_db (void)
{
	return &passwd_db;
}

static int pw_cmp (const void *p1, const void *p2)
{
	uid_t u1, u2;

	if ((*(struct commonio_entry **) p1)->eptr == NULL)
		return 1;
	if ((*(struct commonio_entry **) p2)->eptr == NULL)
		return -1;

	u1 = ((struct passwd *) (*(struct commonio_entry **) p1)->eptr)->pw_uid;
	u2 = ((struct passwd *) (*(struct commonio_entry **) p2)->eptr)->pw_uid;

	if (u1 < u2)
		return -1;
	else if (u1 > u2)
		return 1;
	else
		return 0;
}

/* Sort entries by UID */
int pw_sort ()
{
	return commonio_sort (&passwd_db, pw_cmp);
}
