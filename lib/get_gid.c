/*
 * SPDX-FileCopyrightText: 2009, Nicolas François
 * SPDX-FileCopyrightText: 2023, Alejandro Colomar <alx@kernel.org>
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <config.h>

#ident "$Id$"

#include <inttypes.h>
#include <stddef.h>
#include <sys/types.h>

#include "atoi/strtoi.h"
#include "prototypes.h"
#include "types.h"


int
get_gid(const char *gidstr, gid_t *gid)
{
	int    status;
	gid_t  val;

	val = strtonl(gidstr, NULL, 10, &status, gid_t);
	if (status != 0)
		return -1;

	*gid = val;
	return 0;
}
