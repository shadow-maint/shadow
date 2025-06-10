// SPDX-FileCopyrightText: 1990-1994, Julianne Frances Haugh
// SPDX-FileCopyrightText: 1996-1998, Marek Michałkiewicz
// SPDX-FileCopyrightText: 2005, Tomasz Kłoczko
// SPDX-FileCopyrightText: 2008-2009, Nicolas François
// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include "shadow/gshadow/sgetsgent.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "shadow/gshadow/sgrp.h"
#include "string/strcmp/streq.h"
#include "string/strtok/astrsep2ls.h"
#include "string/strtok/stpsep.h"
#include "string/strtok/strsep2arr.h"


#if defined(SHADOWGRP) && !__has_include(<gshadow.h>)
static struct sgrp  sgroup = {};


static char **build_list(char *s);


// from-string get shadow group entry
struct sgrp *
sgetsgent(const char *s)
{
	static char  *dup = NULL;

	char  *fields[4];

	free(dup);
	dup = strdup(s);
	if (dup == NULL)
		return NULL;

	stpsep(dup, "\n");

	if (strsep2arr_a(dup, ":", fields) == -1)
		return NULL;

	sgroup.sg_namp = fields[0];
	sgroup.sg_passwd = fields[1];

	free(sgroup.sg_adm);
	free(sgroup.sg_mem);

	sgroup.sg_adm = build_list(fields[2]);
	sgroup.sg_mem = build_list(fields[3]);

	if (sgroup.sg_adm == NULL || sgroup.sg_mem == NULL)
		return NULL;

	return &sgroup;
}


static char **
build_list(char *s)
{
	char    **l;
	size_t  n;

	l = astrsep2ls(s, ",", &n);
	if (l == NULL)
		return NULL;

	if (streq(l[n-1], ""))
		l[n-1] = NULL;

	return l;
}
#endif
