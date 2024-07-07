/*
 * SPDX-FileCopyrightText: 1990 - 1994, Julianne Frances Haugh
 * SPDX-FileCopyrightText: 1996 - 2000, Marek Michałkiewicz
 * SPDX-FileCopyrightText: 2001       , Michał Moskal
 * SPDX-FileCopyrightText: 2005       , Tomasz Kłoczko
 * SPDX-FileCopyrightText: 2007 - 2009, Nicolas François
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <config.h>

#ident "$Id$"

#include "prototypes.h"
#include "defines.h"
#include <shadow.h>
#include <stdio.h>
#include "commonio.h"
#include "getdef.h"
#include "shadowio.h"

static /*@null@*/ /*@only@*/void *shadow_dup (const void *ent)
{
	const struct spwd *sp = ent;

	return __spw_dup (sp);
}

static void
shadow_free(/*@only@*/void *ent)
{
	struct spwd *sp = ent;

	spw_free (sp);
}

static const char *shadow_getname (const void *ent)
{
	const struct spwd *sp = ent;

	return sp->sp_namp;
}

static void *shadow_parse (const char *line)
{
	return sgetspent (line);
}

static int shadow_put (const void *ent, FILE * file)
{
	const struct spwd *sp = ent;

	if (   (NULL == sp)
	    || (valid_field (sp->sp_namp, ":\n") == -1)
	    || (valid_field (sp->sp_pwdp, ":\n") == -1)
	    || (strlen (sp->sp_namp) + strlen (sp->sp_pwdp) +
	        1000 > PASSWD_ENTRY_MAX_LENGTH)) {
		return -1;
	}

	return (putspent (sp, file) == -1) ? -1 : 0;
}

static struct commonio_ops shadow_ops = {
	shadow_dup,
	shadow_free,
	shadow_getname,
	shadow_parse,
	shadow_put,
	fgets,
	fputs,
	NULL,			/* open_hook */
	NULL			/* close_hook */
};

static struct commonio_db shadow_db = {
	SHADOW_FILE,		/* filename */
	&shadow_ops,		/* ops */
	NULL,			/* fp */
#ifdef WITH_SELINUX
	NULL,			/* scontext */
#endif				/* WITH_SELINUX */
	0400,                   /* st_mode */
	0,                      /* st_uid */
	0,                      /* st_gid */
	NULL,			/* head */
	NULL,			/* tail */
	NULL,			/* cursor */
	false,			/* changed */
	false,			/* isopen */
	false,			/* locked */
	false,			/* readonly */
	false			/* setname */
};

int spw_setdbname (const char *filename)
{
	return commonio_setname (&shadow_db, filename);
}

/*@observer@*/const char *spw_dbname (void)
{
	return shadow_db.filename;
}

bool spw_file_present (void)
{
	if (getdef_bool ("FORCE_SHADOW"))
		return true;
	return commonio_present (&shadow_db);
}

int spw_lock (void)
{
	return commonio_lock (&shadow_db);
}

int spw_open (int mode)
{
	return commonio_open (&shadow_db, mode);
}

/*@observer@*/ /*@null@*/const struct spwd *spw_locate (const char *name)
{
	return commonio_locate (&shadow_db, name);
}

int spw_update (const struct spwd *sp)
{
	return commonio_update (&shadow_db, sp);
}

int spw_remove (const char *name)
{
	return commonio_remove (&shadow_db, name);
}

int spw_rewind (void)
{
	return commonio_rewind (&shadow_db);
}

/*@observer@*/ /*@null@*/const struct spwd *spw_next (void)
{
	return commonio_next (&shadow_db);
}

int spw_close (void)
{
	return commonio_close (&shadow_db);
}

int spw_unlock (void)
{
	return commonio_unlock (&shadow_db);
}

struct commonio_entry *__spw_get_head (void)
{
	return shadow_db.head;
}

void __spw_del_entry (const struct commonio_entry *ent)
{
	commonio_del_entry (&shadow_db, ent);
}

/* Sort with respect to passwd ordering. */
int spw_sort ()
{
	return commonio_sort_wrt (&shadow_db, __pw_get_db ());
}
