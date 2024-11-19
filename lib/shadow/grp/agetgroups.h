// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_SHADOW_GRP_AGETGROUPS_H_
#define SHADOW_INCLUDE_LIB_SHADOW_GRP_AGETGROUPS_H_


#include <config.h>

#include <stddef.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "alloc/malloc.h"
#include "attr.h"


ATTR_ACCESS(write_only, 1)
ATTR_MALLOC(free)
inline gid_t *agetgroups(size_t *ngids);


// Like getgroups(3), but allocate the buffer.
// *ngids is used to return the number of elements in the allocated array.
inline gid_t *
agetgroups(size_t *ngids)
{
	int    n;
	gid_t  *gids;

	n = getgroups(0, NULL);
	if (n == -1)
		return NULL;

	gids = MALLOC(n, gid_t);
	if (gids == NULL)
		return NULL;

	n = getgroups(n, gids);
	if (n == -1) {
		free(gids);
		return NULL;
	}

	*ngids = n;
	return gids;
}


#endif  // include guard
