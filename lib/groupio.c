
#include <config.h>

#include "rcsid.h"
RCSID ("$Id: groupio.c,v 1.12 2005/06/20 09:56:37 kloczek Exp $")
#include "prototypes.h"
#include "defines.h"
#include "commonio.h"
#include "groupio.h"
extern int putgrent (const struct group *, FILE *);
extern struct group *sgetgrent (const char *);

struct group *__gr_dup (const struct group *grent)
{
	struct group *gr;
	int i;

	if (!(gr = (struct group *) malloc (sizeof *gr)))
		return NULL;
	*gr = *grent;
	if (!(gr->gr_name = strdup (grent->gr_name)))
		return NULL;
	if (!(gr->gr_passwd = strdup (grent->gr_passwd)))
		return NULL;

	for (i = 0; grent->gr_mem[i]; i++);
	gr->gr_mem = (char **) malloc ((i + 1) * sizeof (char *));
	if (!gr->gr_mem)
		return NULL;
	for (i = 0; grent->gr_mem[i]; i++) {
		gr->gr_mem[i] = strdup (grent->gr_mem[i]);
		if (!gr->gr_mem[i])
			return NULL;
	}
	gr->gr_mem[i] = NULL;
	return gr;
}

static void *group_dup (const void *ent)
{
	const struct group *gr = ent;

	return __gr_dup (gr);
}

static void group_free (void *ent)
{
	struct group *gr = ent;

	free (gr->gr_name);
	free (gr->gr_passwd);
	while (*(gr->gr_mem)) {
		free (*(gr->gr_mem));
		gr->gr_mem++;
	}
	free (gr);
}

static const char *group_getname (const void *ent)
{
	const struct group *gr = ent;

	return gr->gr_name;
}

static void *group_parse (const char *line)
{
	return (void *) sgetgrent (line);
}

static int group_put (const void *ent, FILE * file)
{
	const struct group *gr = ent;

	return (putgrent (gr, file) == -1) ? -1 : 0;
}

static struct commonio_ops group_ops = {
	group_dup,
	group_free,
	group_getname,
	group_parse,
	group_put,
	fgetsx,
	fputsx
};

static struct commonio_db group_db = {
	GROUP_FILE,		/* filename */
	&group_ops,		/* ops */
	NULL,			/* fp */
	NULL,			/* head */
	NULL,			/* tail */
	NULL,			/* cursor */
	0,			/* changed */
	0,			/* isopen */
	0,			/* locked */
	0			/* readonly */
};

int gr_name (const char *filename)
{
	return commonio_setname (&group_db, filename);
}

int gr_lock (void)
{
	return commonio_lock (&group_db);
}

int gr_open (int mode)
{
	return commonio_open (&group_db, mode);
}

const struct group *gr_locate (const char *name)
{
	return commonio_locate (&group_db, name);
}

int gr_update (const struct group *gr)
{
	return commonio_update (&group_db, (const void *) gr);
}

int gr_remove (const char *name)
{
	return commonio_remove (&group_db, name);
}

int gr_rewind (void)
{
	return commonio_rewind (&group_db);
}

const struct group *gr_next (void)
{
	return commonio_next (&group_db);
}

int gr_close (void)
{
	return commonio_close (&group_db);
}

int gr_unlock (void)
{
	return commonio_unlock (&group_db);
}

void __gr_set_changed (void)
{
	group_db.changed = 1;
}

struct commonio_entry *__gr_get_head (void)
{
	return group_db.head;
}

struct commonio_db *__gr_get_db (void)
{
	return &group_db;
}

void __gr_del_entry (const struct commonio_entry *ent)
{
	commonio_del_entry (&group_db, ent);
}

static int gr_cmp (const void *p1, const void *p2)
{
	gid_t u1, u2;

	if ((*(struct commonio_entry **) p1)->eptr == NULL)
		return 1;
	if ((*(struct commonio_entry **) p2)->eptr == NULL)
		return -1;

	u1 = ((struct group *) (*(struct commonio_entry **) p1)->eptr)->gr_gid;
	u2 = ((struct group *) (*(struct commonio_entry **) p2)->eptr)->gr_gid;

	if (u1 < u2)
		return -1;
	else if (u1 > u2)
		return 1;
	else
		return 0;
}

/* Sort entries by GID */
int gr_sort ()
{
	return commonio_sort (&group_db, gr_cmp);
}
