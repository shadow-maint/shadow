// SPDX-FileCopyrightText: 1990-1994, Julianne Frances Haugh
// SPDX-FileCopyrightText: 1996-1998, Marek Michałkiewicz
// SPDX-FileCopyrightText: 2005, Tomasz Kłoczko
// SPDX-FileCopyrightText: 2008, Nicolas François
// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include "shadow/group/sgetgrent.h"

#include <errno.h>
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
#include "string/strcpy/stpecpy.h"
#include "string/strtok/stpsep.h"
#include "string/strtok/strsep2arr.h"
#include "typetraits.h"


// from-string get group entry
struct group *
sgetgrent(const char *s)
{
	static char          *buf = NULL;
	static struct group  grent_ = {};
	struct group         *grent = &grent_;

	int     e;
	size_t  n, lssize, size;

	n = strchrcnt(s, ',') + 2;
	lssize = n * sizeof(char *);  // For 'grent->gr_mem'.
	size = lssize + strlen(s) + 1;

	free(buf);
	buf = MALLOC(size, char);
	if (buf == NULL)
		return NULL;

	e = sgetgrent_r(s, grent, buf, size);
	if (e != 0) {
		errno = e;
		return NULL;
	}

	return grent;
}


// from-string get group entry re-entrant
int
sgetgrent_r(size_t size;
    const char *restrict s, struct group *restrict grent,
    char buf[restrict size], size_t size)
{
	char    *p, *end;
	char    *fields[4];
	size_t  n, lssize;

	// The first 'lssize' bytes of 'buf' are used for 'grent->gr_mem'.
	n = strchrcnt(s, ',') + 2;
	lssize = n * sizeof(char *);
	if (lssize >= size)
		return E2BIG;

	// The remaining bytes of 'buf' are used for a copy of 's'.
	end = buf + size;
	p = buf + lssize;
	if (stpecpy(p, end, s) == NULL)
		return errno;

	stpsep(p, "\n");

	if (strsep2arr_a(p, ":", fields) == -1)
		return EINVAL;

	grent->gr_name = fields[0];
	grent->gr_passwd = fields[1];
	if (get_gid(fields[2], &grent->gr_gid) == -1)
		return errno;

	if (!is_aligned(buf, char *))
		return EINVAL;
	grent->gr_mem = (char **) buf;
	if (csv2ls(fields[3], n, grent->gr_mem) == -1)
		return errno;

	return 0;
}
