/*
 * Copyright (c) 1990 - 1994, Julianne Frances Haugh
 * Copyright (c) 1996 - 2000, Marek Michałkiewicz
 * Copyright (c) 2001       , Michał Moskal
 * Copyright (c) 2005       , Tomasz Kłoczko
 * Copyright (c) 2007 - 2010, Nicolas François
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
#include "groupio.h"

/*@null@*/ /*@only@*/struct group *__gr_dup (const struct group *grent)
{
	struct group *gr;
	int i;

	gr = (struct group *) malloc (sizeof *gr);
	if (NULL == gr) {
		return NULL;
	}
	gr->gr_gid = grent->gr_gid;
	gr->gr_name = strdup (grent->gr_name);
	if (NULL == gr->gr_name) {
		free(gr);
		return NULL;
	}
	gr->gr_passwd = strdup (grent->gr_passwd);
	if (NULL == gr->gr_passwd) {
		free(gr->gr_name);
		free(gr);
		return NULL;
	}

	for (i = 0; grent->gr_mem[i]; i++);

	gr->gr_mem = (char **) malloc ((i + 1) * sizeof (char *));
	if (NULL == gr->gr_mem) {
		free(gr->gr_passwd);
		free(gr->gr_name);
		free(gr);
		return NULL;
	}
	for (i = 0; grent->gr_mem[i]; i++) {
		gr->gr_mem[i] = strdup (grent->gr_mem[i]);
		if (NULL == gr->gr_mem[i]) {
			int j;
			for (j=0; j<i; j++)
				free(gr->gr_mem[j]);
			free(gr->gr_mem);
			free(gr->gr_passwd);
			free(gr->gr_name);
			free(gr);
			return NULL;
		}
	}
	gr->gr_mem[i] = NULL;

	return gr;
}

void gr_free (/*@out@*/ /*@only@*/struct group *grent)
{
	free (grent->gr_name);
	if (NULL != grent->gr_passwd) {
		memzero (grent->gr_passwd, strlen (grent->gr_passwd));
		free (grent->gr_passwd);
	}
	if (NULL != grent->gr_mem) {
		size_t i;
		for (i = 0; NULL != grent->gr_mem[i]; i++) {
			free (grent->gr_mem[i]);
		}
		free (grent->gr_mem);
	}
	free (grent);
}

