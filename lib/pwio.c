
#include <config.h>

#ident "$Id$"

#include "prototypes.h"
#include "defines.h"
#include <pwd.h>
#include <stdio.h>
#include "commonio.h"
#include "pwio.h"
extern struct passwd *sgetpwent (const char *);
extern int putpwent (const struct passwd *, FILE *);

struct passwd *__pw_dup (const struct passwd *pwent)
{
	struct passwd *pw;

	if (!(pw = (struct passwd *) malloc (sizeof *pw)))
		return NULL;
	*pw = *pwent;
	if (!(pw->pw_name = strdup (pwent->pw_name)))
		return NULL;
	if (!(pw->pw_passwd = strdup (pwent->pw_passwd)))
		return NULL;
	if (!(pw->pw_gecos = strdup (pwent->pw_gecos)))
		return NULL;
	if (!(pw->pw_dir = strdup (pwent->pw_dir)))
		return NULL;
	if (!(pw->pw_shell = strdup (pwent->pw_shell)))
		return NULL;
	return pw;
}

static void *passwd_dup (const void *ent)
{
	const struct passwd *pw = ent;

	return __pw_dup (pw);
}

static void passwd_free (void *ent)
{
	struct passwd *pw = ent;

	free (pw->pw_name);
	free (pw->pw_passwd);
	free (pw->pw_gecos);
	free (pw->pw_dir);
	free (pw->pw_shell);
	free (pw);
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

	return (putpwent (pw, file) == -1) ? -1 : 0;
}

static struct commonio_ops passwd_ops = {
	passwd_dup,
	passwd_free,
	passwd_getname,
	passwd_parse,
	passwd_put,
	fgets,
	fputs
};

static struct commonio_db passwd_db = {
	PASSWD_FILE,		/* filename */
	&passwd_ops,		/* ops */
	NULL,			/* fp */
	NULL,			/* head */
	NULL,			/* tail */
	NULL,			/* cursor */
	0,			/* changed */
	0,			/* isopen */
	0,			/* locked */
	0			/* readonly */
};

int pw_name (const char *filename)
{
	return commonio_setname (&passwd_db, filename);
}

int pw_lock (void)
{
	return commonio_lock (&passwd_db);
}

int pw_open (int mode)
{
	return commonio_open (&passwd_db, mode);
}

const struct passwd *pw_locate (const char *name)
{
	return commonio_locate (&passwd_db, name);
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

const struct passwd *pw_next (void)
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

struct commonio_entry *__pw_get_head (void)
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
