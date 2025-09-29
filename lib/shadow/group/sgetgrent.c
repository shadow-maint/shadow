// SPDX-FileCopyrightText: 1990-1994, Julianne Frances Haugh
// SPDX-FileCopyrightText: 1996-1998, Marek Michałkiewicz
// SPDX-FileCopyrightText: 2005, Tomasz Kłoczko
// SPDX-FileCopyrightText: 2008, Nicolas François
// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include "shadow/group/sgetgrent.h"

#include <grp.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "alloc/malloc.h"
#include "atoi/getnum.h"
#include "list.h"
#include "string/strchr/strchrcnt.h"
#include "string/strtok/stpsep.h"
#include "string/strtok/strsep2arr.h"


// from-string get group entry
struct group *
sgetgrent(const char *s)
{
	static char         **buf = NULL;
	static char         *p = NULL;
	static struct group grent = {};

	char    *fields[4];
	size_t  n;

	n = strchrcnt(s, ',') + 2;

	free(buf);
	buf = MALLOC(n, char *);
	if (buf == NULL)
		return NULL;

	free(p);
	p = strdup(s);
	if (p == NULL)
		return NULL;

	stpsep(p, "\n");

	if (strsep2arr_a(p, ":", fields) == -1)
		return NULL;

	grent.gr_name = fields[0];
	grent.gr_passwd = fields[1];
	if (get_gid(fields[2], &grent.gr_gid) == -1)
		return NULL;

	grent.gr_mem = buf;
	if (csv2ls(fields[3], n, grent.gr_mem) == -1)
		return NULL;

	return &grent;
}
