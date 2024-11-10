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
#include "string/strtok/stpsep.h"
#include "string/strtok/strsep2arr.h"
#include "string/strtok/xastrsep2ls.h"


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

	if (STRSEP2ARR(dup, ":", fields) == -1)
		return NULL;

	sgroup.sg_namp = fields[0];
	sgroup.sg_passwd = fields[1];

	free(sgroup.sg_adm);
	free(sgroup.sg_mem);

	sgroup.sg_adm = build_list(fields[2]);
	sgroup.sg_mem = build_list(fields[3]);

	return &sgroup;
}


static char **
build_list(char *s)
{
	char    **l;
	size_t  n;

	l = xastrsep2ls(s, ",", &n);

	if (streq(l[n-1], ""))
		l[n-1] = NULL;

	return l;
}
#endif
