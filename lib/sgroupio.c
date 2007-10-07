
#include <config.h>

#ifdef SHADOWGRP

#include "rcsid.h"
RCSID("$Id: sgroupio.c,v 1.12 2001/08/14 21:10:36 malekith Exp $")

#include "prototypes.h"
#include "defines.h"

#include "commonio.h"
#include "sgroupio.h"

extern int putsgent(const struct sgrp *, FILE *);
extern struct sgrp *sgetsgent(const char *);

struct sgrp *
__sgr_dup(const struct sgrp *sgent)
{
	struct sgrp *sg;
	int i;

	if (!(sg = (struct sgrp *) malloc(sizeof *sg)))
		return NULL;
	*sg = *sgent;
	if (!(sg->sg_name = strdup(sgent->sg_name)))
		return NULL;
	if (!(sg->sg_passwd = strdup(sgent->sg_passwd)))
		return NULL;

	for (i = 0; sgent->sg_adm[i]; i++)
		;
	sg->sg_adm = (char **) malloc((i + 1) * sizeof(char *));
	if (!sg->sg_adm)
		return NULL;
	for (i = 0; sgent->sg_adm[i]; i++) {
		sg->sg_adm[i] = strdup(sgent->sg_adm[i]);
		if (!sg->sg_adm[i])
			return NULL;
	}
	sg->sg_adm[i] = NULL;

	for (i = 0; sgent->sg_mem[i]; i++)
		;
	sg->sg_mem = (char **) malloc((i + 1) * sizeof(char *));
	if (!sg->sg_mem)
		return NULL;
	for (i = 0; sgent->sg_mem[i]; i++) {
		sg->sg_mem[i] = strdup(sgent->sg_mem[i]);
		if (!sg->sg_mem[i])
			return NULL;
	}
	sg->sg_mem[i] = NULL;

	return sg;
}

static void *
gshadow_dup(const void *ent)
{
	const struct sgrp *sg = ent;
	return __sgr_dup(sg);
}

static void
gshadow_free(void *ent)
{
	struct sgrp *sg = ent;

	free(sg->sg_name);
	free(sg->sg_passwd);
	while(*(sg->sg_adm)) {
		free(*(sg->sg_adm));
		sg->sg_adm++;
	}
	while(*(sg->sg_mem)) {
		free(*(sg->sg_mem));
		sg->sg_mem++;
	}
	free(sg);
}

static const char *
gshadow_getname(const void *ent)
{
	const struct sgrp *gr = ent;
	return gr->sg_name;
}

static void *
gshadow_parse(const char *line)
{
	return (void *) sgetsgent(line);
}

static int
gshadow_put(const void *ent, FILE *file)
{
	const struct sgrp *sg = ent;
	return (putsgent(sg, file) == -1) ? -1 : 0;
}

static struct commonio_ops gshadow_ops = {
	gshadow_dup,
	gshadow_free,
	gshadow_getname,
	gshadow_parse,
	gshadow_put,
	fgetsx,
	fputsx
};

static struct commonio_db gshadow_db = {
	SGROUP_FILE,	/* filename */
	&gshadow_ops,	/* ops */
	NULL,		/* fp */
	NULL,		/* head */
	NULL,		/* tail */
	NULL,		/* cursor */
	0,		/* changed */
	0,		/* isopen */
	0,		/* locked */
	0		/* readonly */
};

int
sgr_name(const char *filename)
{
	return commonio_setname(&gshadow_db, filename);
}

int
sgr_file_present(void)
{
	return commonio_present(&gshadow_db);
}

int
sgr_lock(void)
{
	return commonio_lock(&gshadow_db);
}

int
sgr_open(int mode)
{
	return commonio_open(&gshadow_db, mode);
}

const struct sgrp *
sgr_locate(const char *name)
{
	return commonio_locate(&gshadow_db, name);
}

int
sgr_update(const struct sgrp *sg)
{
	return commonio_update(&gshadow_db, (const void *) sg);
}

int
sgr_remove(const char *name)
{
	return commonio_remove(&gshadow_db, name);
}

int
sgr_rewind(void)
{
	return commonio_rewind(&gshadow_db);
}

const struct sgrp *
sgr_next(void)
{
	return commonio_next(&gshadow_db);
}

int
sgr_close(void)
{
	return commonio_close(&gshadow_db);
}

int
sgr_unlock(void)
{
	return commonio_unlock(&gshadow_db);
}

void
__sgr_set_changed(void)
{
	gshadow_db.changed = 1;
}

struct commonio_entry *
__sgr_get_head(void)
{
	return gshadow_db.head;
}

void
__sgr_del_entry(const struct commonio_entry *ent)
{
	commonio_del_entry(&gshadow_db, ent);
}

/* Sort with respect to group ordering. */
int
sgr_sort()
{
	extern struct commonio_db *__gr_get_db();

	return commonio_sort_wrt(&gshadow_db, __gr_get_db());
}
#else
extern int errno;  /* warning: ANSI C forbids an empty source file */
#endif
