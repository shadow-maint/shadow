// SPDX-FileCopyrightText: 1990-1994, Julianne Frances Haugh
// SPDX-FileCopyrightText: 1996-1998, Marek Michałkiewicz
// SPDX-FileCopyrightText: 2005, Tomasz Kłoczko
// SPDX-FileCopyrightText: 2008-2009, Nicolas François
// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include "shadow/gshadow/putsgent.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "alloc/malloc.h"
#include "prototypes.h"
#include "shadow/gshadow/sgrp.h"


/*
 * putsgent - output shadow group entry in text form
 *
 * putsgent() converts the contents of a (struct sgrp) to text and
 * writes the result to the given stream.  This is the logical
 * opposite of fgetsgent.
 */
#if defined(SHADOWGRP) && !__has_include(<gshadow.h>)
// put shadow group entry
int
putsgent(const struct sgrp *sgrp, FILE *fp)
{
	char *buf, *cp;
	int i;
	size_t size;

	if ((NULL == fp) || (NULL == sgrp)) {
		return -1;
	}

	/* calculate the required buffer size */
	size = strlen (sgrp->sg_namp) + strlen (sgrp->sg_passwd) + 10;
	for (i = 0; (NULL != sgrp->sg_adm) && (NULL != sgrp->sg_adm[i]); i++) {
		size += strlen (sgrp->sg_adm[i]) + 1;
	}
	for (i = 0; (NULL != sgrp->sg_mem) && (NULL != sgrp->sg_mem[i]); i++) {
		size += strlen (sgrp->sg_mem[i]) + 1;
	}

	buf = MALLOC(size, char);
	if (NULL == buf) {
		return -1;
	}
	cp = buf;

	/*
	 * Copy the group name and passwd.
	 */
	cp = stpcpy(stpcpy(cp, sgrp->sg_namp), ":");
	cp = stpcpy(stpcpy(cp, sgrp->sg_passwd), ":");

	/*
	 * Copy the administrators, separating each from the other
	 * with a ",".
	 */
	for (i = 0; NULL != sgrp->sg_adm[i]; i++) {
		if (i > 0)
			cp = stpcpy(cp, ",");

		cp = stpcpy(cp, sgrp->sg_adm[i]);
	}
	cp = stpcpy(cp, ":");

	/*
	 * Now do likewise with the group members.
	 */
	for (i = 0; NULL != sgrp->sg_mem[i]; i++) {
		if (i > 0)
			cp = stpcpy(cp, ",");

		cp = stpcpy(cp, sgrp->sg_mem[i]);
	}
	stpcpy(cp, "\n");

	/*
	 * Output using the function which understands the line
	 * continuation conventions.
	 */
	if (fputsx (buf, fp) == EOF) {
		free (buf);
		return -1;
	}

	free (buf);
	return 0;
}
#endif
