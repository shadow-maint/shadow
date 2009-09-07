/*
 * Copyright (c) 1990 - 1994, Julianne Frances Haugh
 * Copyright (c) 1996 - 2000, Marek Michałkiewicz
 * Copyright (c) 2001       , Michał Moskal
 * Copyright (c) 2005       , Tomasz Kłoczko
 * Copyright (c) 2007 - 2008, Nicolas François
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

#ifdef SHADOWGRP

#ident "$Id$"

#include "prototypes.h"
#include "defines.h"
#include "commonio.h"
#include "sgroupio.h"

/*@null@*/ /*@only@*/struct sgrp *__sgr_dup (const struct sgrp *sgent)
{
	struct sgrp *sg;
	int i;

	sg = (struct sgrp *) malloc (sizeof *sg);
	if (NULL == sg) {
		return NULL;
	}
	sg->sg_name = strdup (sgent->sg_name);
	if (NULL == sg->sg_name) {
		free (sg);
		return NULL;
	}
	sg->sg_passwd = strdup (sgent->sg_passwd);
	if (NULL == sg->sg_passwd) {
		free (sg->sg_name);
		free (sg);
		return NULL;
	}

	for (i = 0; NULL != sgent->sg_adm[i]; i++);
	sg->sg_adm = (char **) malloc ((i + 1) * sizeof (char *));
	if (NULL == sg->sg_adm) {
		free (sg->sg_passwd);
		free (sg->sg_name);
		free (sg);
		return NULL;
	}
	for (i = 0; NULL != sgent->sg_adm[i]; i++) {
		sg->sg_adm[i] = strdup (sgent->sg_adm[i]);
		if (NULL == sg->sg_adm[i]) {
			for (i = 0; NULL != sg->sg_adm[i]; i++) {
				free (sg->sg_adm[i]);
			}
			free (sg->sg_adm);
			free (sg->sg_passwd);
			free (sg->sg_name);
			free (sg);
			return NULL;
		}
	}
	sg->sg_adm[i] = NULL;

	for (i = 0; NULL != sgent->sg_mem[i]; i++);
	sg->sg_mem = (char **) malloc ((i + 1) * sizeof (char *));
	if (NULL == sg->sg_mem) {
		for (i = 0; NULL != sg->sg_adm[i]; i++) {
			free (sg->sg_adm[i]);
		}
		free (sg->sg_adm);
		free (sg->sg_passwd);
		free (sg->sg_name);
		free (sg);
		return NULL;
	}
	for (i = 0; NULL != sgent->sg_mem[i]; i++) {
		sg->sg_mem[i] = strdup (sgent->sg_mem[i]);
		if (NULL == sg->sg_mem[i]) {
			for (i = 0; NULL != sg->sg_mem[i]; i++) {
				free (sg->sg_mem[i]);
			}
			free (sg->sg_mem);
			for (i = 0; NULL != sg->sg_adm[i]; i++) {
				free (sg->sg_adm[i]);
			}
			free (sg->sg_adm);
			free (sg->sg_passwd);
			free (sg->sg_name);
			free (sg);
			return NULL;
		}
	}
	sg->sg_mem[i] = NULL;

	return sg;
}

static /*@null@*/ /*@only@*/void *gshadow_dup (const void *ent)
{
	const struct sgrp *sg = ent;

	return __sgr_dup (sg);
}

static void gshadow_free (/*@out@*/ /*@only@*/void *ent)
{
	struct sgrp *sg = ent;

	sgr_free (sg);
}

void sgr_free (/*@out@*/ /*@only@*/struct sgrp *sgent)
{
	size_t i;
	free (sgent->sg_name);
	if (NULL != sgent->sg_passwd) {
		memzero (sgent->sg_passwd, strlen (sgent->sg_passwd));
		free (sgent->sg_passwd);
	}
	for (i = 0; NULL != sgent->sg_adm[i]; i++) {
		free (sgent->sg_adm[i]);
	}
	free (sgent->sg_adm);
	for (i = 0; NULL != sgent->sg_mem[i]; i++) {
		free (sgent->sg_mem[i]);
	}
	free (sgent->sg_mem);
	free (sgent);
}

static const char *gshadow_getname (const void *ent)
{
	const struct sgrp *gr = ent;

	return gr->sg_name;
}

static void *gshadow_parse (const char *line)
{
	return (void *) sgetsgent (line);
}

static int gshadow_put (const void *ent, FILE * file)
{
	const struct sgrp *sg = ent;

	return (putsgent (sg, file) == -1) ? -1 : 0;
}

static struct commonio_ops gshadow_ops = {
	gshadow_dup,
	gshadow_free,
	gshadow_getname,
	gshadow_parse,
	gshadow_put,
	fgetsx,
	fputsx,
	NULL,			/* open_hook */
	NULL			/* close_hook */
};

static struct commonio_db gshadow_db = {
	SGROUP_FILE,		/* filename */
	&gshadow_ops,		/* ops */
	NULL,			/* fp */
#ifdef WITH_SELINUX
	NULL,			/* scontext */
#endif
	NULL,			/* head */
	NULL,			/* tail */
	NULL,			/* cursor */
	false,			/* changed */
	false,			/* isopen */
	false,			/* locked */
	false			/* readonly */
};

int sgr_setdbname (const char *filename)
{
	return commonio_setname (&gshadow_db, filename);
}

/*@observer@*/const char *sgr_dbname (void)
{
	return gshadow_db.filename;
}

bool sgr_file_present (void)
{
	return commonio_present (&gshadow_db);
}

int sgr_lock (void)
{
	return commonio_lock (&gshadow_db);
}

int sgr_open (int mode)
{
	return commonio_open (&gshadow_db, mode);
}

/*@observer@*/ /*@null@*/const struct sgrp *sgr_locate (const char *name)
{
	return commonio_locate (&gshadow_db, name);
}

int sgr_update (const struct sgrp *sg)
{
	return commonio_update (&gshadow_db, (const void *) sg);
}

int sgr_remove (const char *name)
{
	return commonio_remove (&gshadow_db, name);
}

int sgr_rewind (void)
{
	return commonio_rewind (&gshadow_db);
}

/*@null@*/const struct sgrp *sgr_next (void)
{
	return commonio_next (&gshadow_db);
}

int sgr_close (void)
{
	return commonio_close (&gshadow_db);
}

int sgr_unlock (void)
{
	return commonio_unlock (&gshadow_db);
}

void __sgr_set_changed (void)
{
	gshadow_db.changed = true;
}

/*@dependent@*/ /*@null@*/struct commonio_entry *__sgr_get_head (void)
{
	return gshadow_db.head;
}

void __sgr_del_entry (const struct commonio_entry *ent)
{
	commonio_del_entry (&gshadow_db, ent);
}

/* Sort with respect to group ordering. */
int sgr_sort ()
{
	return commonio_sort_wrt (&gshadow_db, __gr_get_db ());
}
#else
extern int errno;		/* warning: ANSI C forbids an empty source file */
#endif
