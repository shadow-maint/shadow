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

static const char *Prog = "(libsubid)";
static FILE *shadow_logfd;

bool libsubid_init(const char *progname, FILE * logfd)
{
	if (progname) {
		progname = strdup(progname);
		if (progname)
			Prog = progname;
		else
			return false;
	}

	if (logfd) {
		shadow_logfd = logfd;
		return true;
	}
	shadow_logfd = fopen("/dev/null", "w");
	if (!shadow_logfd) {
		shadow_logfd = stderr;
		return false;
	}
	return true;
}

static
int get_subid_ranges(const char *owner, enum subid_type id_type, struct subid_range **ranges)
{
	return list_owner_ranges(owner, id_type, ranges);
}

int get_subuid_ranges(const char *owner, struct subid_range **ranges)
{
	return get_subid_ranges(owner, ID_TYPE_UID, ranges);
}

int get_subgid_ranges(const char *owner, struct subid_range **ranges)
{
	return get_subid_ranges(owner, ID_TYPE_GID, ranges);
}

static
int get_subid_owner(unsigned long id, enum subid_type id_type, uid_t **owner)
{
	return find_subid_owners(id, id_type, owner);
}

int get_subuid_owners(uid_t uid, uid_t **owner)
{
	return get_subid_owner((unsigned long)uid, ID_TYPE_UID, owner);
}

int get_subgid_owners(gid_t gid, uid_t **owner)
{
	return get_subid_owner((unsigned long)gid, ID_TYPE_GID, owner);
}

static
bool grant_subid_range(struct subordinate_range *range, bool reuse,
		       enum subid_type id_type)
{
	return new_subid_range(range, id_type, reuse);
}

bool grant_subuid_range(struct subordinate_range *range, bool reuse)
{
	return grant_subid_range(range, reuse, ID_TYPE_UID);
}

bool grant_subgid_range(struct subordinate_range *range, bool reuse)
{
	return grant_subid_range(range, reuse, ID_TYPE_GID);
}

static
bool ungrant_subid_range(struct subordinate_range *range, enum subid_type id_type)
{
	return release_subid_range(range, id_type);
}

bool ungrant_subuid_range(struct subordinate_range *range)
{
	return ungrant_subid_range(range, ID_TYPE_UID);
}

bool ungrant_subgid_range(struct subordinate_range *range)
{
	return ungrant_subid_range(range, ID_TYPE_GID);
}
