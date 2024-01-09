// SPDX-FileCopyrightText: 2009, Nicolas François
// SPDX-FileCopyrightText: 2023, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include <config.h>

#ident "$Id$"

#include "atoi/a2i.h"
#include "prototypes.h"


int
get_uid(const char *uidstr, uid_t *uid)
{
	return a2i(uid_t, uid, uidstr, NULL, 10, type_min(uid_t), type_max(uid_t));
}
