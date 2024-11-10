// SPDX-FileCopyrightText: 1990-1994, Julianne Frances Haugh
// SPDX-FileCopyrightText: 1996-1998, Marek Michałkiewicz
// SPDX-FileCopyrightText: 2005, Tomasz Kłoczko
// SPDX-FileCopyrightText: 2008-2009, Nicolas François
// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include "shadow/gshadow/fgetsgent.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "alloc/malloc.h"
#include "alloc/realloc.h"
#include "defines.h"
#include "prototypes.h"
#include "shadow/gshadow/sgetsgent.h"
#include "shadow/gshadow/sgrp.h"
#include "string/strtok/stpsep.h"


/*
 * fgetsgent - convert next line in stream to (struct sgrp)
 *
 * fgetsgent() reads the next line from the provided stream and
 * converts it to a (struct sgrp).  NULL is returned on EOF.
 */
#if defined(SHADOWGRP) && !__has_include(<gshadow.h>)
// from-FILE get-next shadow group entry
struct sgrp *
fgetsgent(FILE *fp)
{
	static size_t buflen = 0;
	static char *buf = NULL;

	char *cp;

	if (0 == buflen) {
		buf = MALLOC(BUFSIZ, char);
		if (NULL == buf) {
			return NULL;
		}
		buflen = BUFSIZ;
	}

	if (NULL == fp) {
		return NULL;
	}

	if (fgetsx(buf, buflen, fp) == NULL)
		return NULL;

	while (   (strrchr(buf, '\n') == NULL)
	       && (feof (fp) == 0)) {
		size_t len;

		cp = REALLOC(buf, buflen * 2, char);
		if (NULL == cp) {
			return NULL;
		}
		buf = cp;
		buflen *= 2;

		len = strlen (buf);
		if (fgetsx (&buf[len],
			    (int) (buflen - len),
			    fp) != &buf[len]) {
			return NULL;
		}
	}
	stpsep(buf, "\n");
	return sgetsgent(buf);
}
#endif
