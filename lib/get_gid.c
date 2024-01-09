// SPDX-FileCopyrightText: 2009, Nicolas François
// SPDX-FileCopyrightText: 2023-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include <config.h>

#ident "$Id$"

#include <sys/types.h>

#include "prototypes.h"
#include "atoi/a2i.h"
#include "typetraits.h"


int
get_gid(const char *gidstr, gid_t *gid)
{
	return a2i(gid_t, gid, gidstr, NULL, 10, type_min(gid_t), type_max(gid_t));
}
