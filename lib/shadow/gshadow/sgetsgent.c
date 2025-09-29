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

#include "alloc/malloc.h"
#include "list.h"
#include "shadow/gshadow/sgrp.h"
#include "string/strchr/strchrcnt.h"
#include "string/strcmp/streq.h"
#include "string/strtok/stpsep.h"
#include "string/strtok/strsep2arr.h"


#if defined(SHADOWGRP) && !__has_include(<gshadow.h>)
// from-string get shadow group entry
struct sgrp *
sgetsgent(const char *s)
{
	static char         **buf = NULL;
	static char         *dup = NULL;
	static struct sgrp  sgroup = {};

	char    *fields[4];
	size_t  n, nadm, nmem;

	n = strchrcnt(s, ',') + 4;

	free(buf);
	buf = MALLOC(n, char *);
	if (buf == NULL)
		return NULL;

	free(dup);
	dup = strdup(s);
	if (dup == NULL)
		return NULL;

	stpsep(dup, "\n");

	if (strsep2arr_a(dup, ":", fields) == -1)
		return NULL;

	sgroup.sg_namp = fields[0];
	sgroup.sg_passwd = fields[1];

	sgroup.sg_adm = buf;
	nadm = strchrcnt(fields[2], ',') + 2;
	if (nadm > n)
		return NULL;
	if (csv2ls(fields[2], nadm, sgroup.sg_adm) == -1)
		return NULL;

	sgroup.sg_mem = buf + nadm;
	nmem = strchrcnt(fields[3], ',') + 2;
	if (nmem + nadm > n)
		return NULL;
	if (csv2ls(fields[3], nmem, sgroup.sg_mem) == -1)
		return NULL;

	return &sgroup;
}
#endif
