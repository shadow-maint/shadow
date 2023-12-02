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
get_uid(const char *uidstr, uid_t *uid)
{
	int    status;
	uid_t  val;

	val = strtonl(uidstr, NULL, 10, &status, uid_t);
	if (status != 0)
		return -1;

	*uid = val;
	return 0;
}

