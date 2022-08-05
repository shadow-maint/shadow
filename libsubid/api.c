/*
 * SPDX-FileCopyrightText: 2020 Serge Hallyn
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <config.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <stdbool.h>
#include "subordinateio.h"
#include "idmapping.h"
#include "subid.h"
#include "shadowlog.h"

static
int get_subid_ranges(const char *owner, enum subid_type id_type, struct subid_range **ranges)
{
	return list_owner_ranges(owner, id_type, ranges);
}

int subid_get_uid_ranges(const char *owner, struct subid_range **ranges)
{
	return get_subid_ranges(owner, ID_TYPE_UID, ranges);
}

int subid_get_gid_ranges(const char *owner, struct subid_range **ranges)
{
	return get_subid_ranges(owner, ID_TYPE_GID, ranges);
}

static
int get_subid_owner(unsigned long id, enum subid_type id_type, uid_t **owner)
{
	return find_subid_owners(id, id_type, owner);
}

int subid_get_uid_owners(uid_t uid, uid_t **owner)
{
	return get_subid_owner((unsigned long)uid, ID_TYPE_UID, owner);
}

int subid_get_gid_owners(gid_t gid, uid_t **owner)
{
	return get_subid_owner((unsigned long)gid, ID_TYPE_GID, owner);
}

static
bool grant_subid_range(struct subordinate_range *range, bool reuse,
		       enum subid_type id_type)
{
	return new_subid_range(range, id_type, reuse);
}

bool subid_grant_uid_range(struct subordinate_range *range, bool reuse)
{
	return grant_subid_range(range, reuse, ID_TYPE_UID);
}

bool subid_grant_gid_range(struct subordinate_range *range, bool reuse)
{
	return grant_subid_range(range, reuse, ID_TYPE_GID);
}

static
bool ungrant_subid_range(struct subordinate_range *range, enum subid_type id_type)
{
	return release_subid_range(range, id_type);
}

bool subid_ungrant_uid_range(struct subordinate_range *range)
{
	return ungrant_subid_range(range, ID_TYPE_UID);
}

bool subid_ungrant_gid_range(struct subordinate_range *range)
{
	return ungrant_subid_range(range, ID_TYPE_GID);
}
