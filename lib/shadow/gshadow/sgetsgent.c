// SPDX-FileCopyrightText: 1990-1994, Julianne Frances Haugh
// SPDX-FileCopyrightText: 1996-1998, Marek Michałkiewicz
// SPDX-FileCopyrightText: 2005, Tomasz Kłoczko
// SPDX-FileCopyrightText: 2008-2009, Nicolas François
// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include <config.h>

#include "shadow/gshadow/sgetsgent.h"

#include <stddef.h>
#include <string.h>

#include "alloc/realloc.h"
#include "alloc/x/xmalloc.h"
#include "shadow/gshadow/sgrp.h"
#include "string/strchr/strchrcnt.h"
#include "string/strtok/stpsep.h"


#define FIELDS	4


#if defined(SHADOWGRP) && !__has_include(<gshadow.h>)
static struct sgrp  sgroup = {};


static char **build_list(char *s);


// from-string get shadow group entry
struct sgrp *
sgetsgent(const char *string)
{
	static char *sgrbuf = NULL;
	static size_t sgrbuflen = 0;

	char *fields[FIELDS];
	char *cp;
	int i;
	size_t len = strlen (string) + 1;

	if (len > sgrbuflen) {
		char *buf = REALLOC(sgrbuf, len, char);
		if (NULL == buf)
			return NULL;

		sgrbuf = buf;
		sgrbuflen = len;
	}

	strcpy (sgrbuf, string);
	stpsep(sgrbuf, "\n");

	/*
	 * There should be exactly 4 colon separated fields.  Find
	 * all 4 of them and save the starting addresses in fields[].
	 */

	for (cp = sgrbuf, i = 0; (i < FIELDS) && (NULL != cp); i++)
		fields[i] = strsep(&cp, ":");

	/*
	 * If there was an extra field somehow, or perhaps not enough,
	 * the line is invalid.
	 */

	if (NULL != cp || i != FIELDS)
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
	size_t  i;

	l = XMALLOC(strchrcnt(s, ',') + 2, char *);

	for (i = 0; s != NULL && !streq(s, ""); i++)
		l[i] = strsep(&s, ",");

	l[i] = NULL;

	return l;
}
#endif
