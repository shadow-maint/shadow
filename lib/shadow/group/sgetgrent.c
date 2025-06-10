// SPDX-FileCopyrightText: 1990-1994, Julianne Frances Haugh
// SPDX-FileCopyrightText: 1996-1998, Marek Michałkiewicz
// SPDX-FileCopyrightText: 2005, Tomasz Kłoczko
// SPDX-FileCopyrightText: 2008, Nicolas François
// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include "shadow/group/sgetgrent.h"

#include <grp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "alloc/malloc.h"
#include "atoi/getnum.h"
#include "defines.h"
#include "prototypes.h"
#include "string/strtok/stpsep.h"
#include "string/strtok/strsep2arr.h"


// from-string get group entry
struct group *
sgetgrent(const char *s)
{
	static char         *dup = NULL;
	static struct group grent = {};

	char  *fields[4];

	free(dup);
	dup = strdup(s);
	if (dup == NULL)
		return NULL;

	stpsep(dup, "\n");

	if (strsep2arr_a(dup, ":", fields) == -1)
		return NULL;

	grent.gr_name = fields[0];
	grent.gr_passwd = fields[1];
	if (get_gid(fields[2], &grent.gr_gid) == -1)
		return NULL;

	free(grent.gr_mem);

	grent.gr_mem = build_list(fields[3]);
	if (NULL == grent.gr_mem) {
		return NULL;	/* out of memory */
	}

	return &grent;
}
