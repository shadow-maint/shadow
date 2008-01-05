
#include <config.h>

#ident "$Id$"

#include "prototypes.h"
#include "defines.h"
#include <shadow.h>
#include <stdio.h>
#include "commonio.h"
#include "shadowio.h"

static void *shadow_dup (const void *ent)
{
	const struct spwd *sp = ent;

	return __spw_dup (sp);
}

static void shadow_free (void *ent)
{
	struct spwd *sp = ent;

	free (sp->sp_namp);
	free (sp->sp_pwdp);
	free (sp);
}

static const char *shadow_getname (const void *ent)
{
	const struct spwd *sp = ent;

	return sp->sp_namp;
}

static void *shadow_parse (const char *line)
{
	return (void *) sgetspent (line);
}

static int shadow_put (const void *ent, FILE * file)
{
	const struct spwd *sp = ent;

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
#endif
	NULL,			/* head */
	NULL,			/* tail */
	NULL,			/* cursor */
	0,			/* changed */
	0,			/* isopen */
	0,			/* locked */
	0			/* readonly */
};

int spw_name (const char *filename)
{
	return commonio_setname (&shadow_db, filename);
}

int spw_file_present (void)
{
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

const struct spwd *spw_locate (const char *name)
{
	return commonio_locate (&shadow_db, name);
}

int spw_update (const struct spwd *sp)
{
	return commonio_update (&shadow_db, (const void *) sp);
}

int spw_remove (const char *name)
{
	return commonio_remove (&shadow_db, name);
}

int spw_rewind (void)
{
	return commonio_rewind (&shadow_db);
}

const struct spwd *spw_next (void)
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
	extern struct commonio_db *__pw_get_db ();

	return commonio_sort_wrt (&shadow_db, __pw_get_db ());
}
