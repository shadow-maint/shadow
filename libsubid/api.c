/*
 * Copyright (c) 2020 Serge Hallyn
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the copyright holders or contributors may not be used to
 *    endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

const char *Prog = "(libsubid)";
extern FILE * shadow_logfd;

bool libsubid_init(const char *progname, FILE * logfd)
{
	if (progname) {
		progname = strdup(progname);
		if (progname)
			Prog = progname;
		else
			fprintf(stderr, "Out of memory");
	}

	if (logfd) {
		shadow_logfd = logfd;
		return true;
	}
	shadow_logfd = fopen("/dev/null", "w");
	if (!shadow_logfd) {
		fprintf(stderr, "ERROR opening /dev/null for error messages.  Using stderr.");
		shadow_logfd = stderr;
		return false;
	}
	return true;
}

static
int get_subid_ranges(const char *owner, enum subid_type id_type, struct subid_range ***ranges)
{
	return list_owner_ranges(owner, id_type, ranges);
}

int get_subuid_ranges(const char *owner, struct subid_range ***ranges)
{
	return get_subid_ranges(owner, ID_TYPE_UID, ranges);
}

int get_subgid_ranges(const char *owner, struct subid_range ***ranges)
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
