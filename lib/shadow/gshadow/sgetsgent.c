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
#include "string/strcpy/stpecpy.h"
#include "string/strtok/stpsep.h"
#include "string/strtok/strsep2arr.h"


#if defined(SHADOWGRP) && !__has_include(<gshadow.h>)
// from-string get shadow group entry
struct sgrp *
sgetsgent(const char *s)
{
	static char         *buf = NULL;
	static struct sgrp  sgent = {};

	char    *fields[4];
	char    *p, *end;
	size_t  n, nadm, nmem, lssize, size;

	n = strchrcnt(s, ',') + 4;
	lssize = n * sizeof(char *);  // For 'sgent.sg_adm' and 'sgent.sg_mem'
	size = lssize + strlen(s) + 1;

	free(buf);
	buf = MALLOC(size, char);
	if (buf == NULL)
		return NULL;

	end = buf + size;
	p = buf + lssize;
	if (stpecpy(p, end, s) == NULL)
		return NULL;

	stpsep(p, "\n");

	if (strsep2arr_a(p, ":", fields) == -1)
		return NULL;

	sgent.sg_namp = fields[0];
	sgent.sg_passwd = fields[1];

	sgent.sg_adm = (char **) buf;
	nadm = strchrcnt(fields[2], ',') + 2;
	if (nadm > n)
		return NULL;
	if (csv2ls(fields[2], nadm, sgent.sg_adm) == -1)
		return NULL;

	sgent.sg_mem = sgent.sg_adm + nadm;
	nmem = strchrcnt(fields[3], ',') + 2;
	if (nmem + nadm > n)
		return NULL;
	if (csv2ls(fields[3], nmem, sgent.sg_mem) == -1)
		return NULL;

	return &sgent;
}
#endif
