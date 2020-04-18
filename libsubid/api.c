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
#include <pwd.h>
#include <stdbool.h>
#include "subordinateio.h"
#include "idmapping.h"
#include "api.h"

static struct subordinate_range **get_subid_ranges(const char *owner, enum subid_type id_type)
{
	struct subordinate_range **ranges = NULL;

	switch (id_type) {
	case ID_TYPE_UID:
		if (!sub_uid_open(O_RDONLY)) {
			return NULL;
		}
		break;
	case ID_TYPE_GID:
		if (!sub_gid_open(O_RDONLY)) {
			return NULL;
		}
		break;
	default:
		return NULL;
	}

	ranges = list_owner_ranges(owner, id_type);

	if (id_type == ID_TYPE_UID)
		sub_uid_close();
	else
		sub_gid_close();

	return ranges;
}

struct subordinate_range **get_subuid_ranges(const char *owner)
{
	return get_subid_ranges(owner, ID_TYPE_UID);
}

struct subordinate_range **get_subgid_ranges(const char *owner)
{
	return get_subid_ranges(owner, ID_TYPE_GID);
}

void subid_free_ranges(struct subordinate_range **ranges)
{
	return free_subordinate_ranges(ranges);
}

int get_subid_owner(unsigned long id, uid_t **owner, enum subid_type id_type)
{
	int ret = -1;

	switch (id_type) {
	case ID_TYPE_UID:
		if (!sub_uid_open(O_RDONLY)) {
			return -1;
		}
		break;
	case ID_TYPE_GID:
		if (!sub_gid_open(O_RDONLY)) {
			return -1;
		}
		break;
	default:
		return -1;
	}

	ret = find_subid_owners(id, owner, id_type);

	if (id_type == ID_TYPE_UID)
		sub_uid_close();
	else
		sub_gid_close();

	return ret;
}

int get_subuid_owners(uid_t uid, uid_t **owner)
{
	return get_subid_owner((unsigned long)uid, owner, ID_TYPE_UID);
}

int get_subgid_owners(gid_t gid, uid_t **owner)
{
	return get_subid_owner((unsigned long)gid, owner, ID_TYPE_GID);
}

bool grant_subid_range(struct subordinate_range *range, bool reuse,
		       enum subid_type id_type)
{
	bool ret;

	switch (id_type) {
	case ID_TYPE_UID:
		if (!sub_uid_lock()) {
			printf("Failed loging subuids (errno %d)\n", errno);
			return false;
		}
		if (!sub_uid_open(O_CREAT | O_RDWR)) {
			printf("Failed opening subuids (errno %d)\n", errno);
			sub_uid_unlock();
			return false;
		}
		break;
	case ID_TYPE_GID:
		if (!sub_gid_lock()) {
			printf("Failed loging subgids (errno %d)\n", errno);
			return false;
		}
		if (!sub_gid_open(O_CREAT | O_RDWR)) {
			printf("Failed opening subgids (errno %d)\n", errno);
			sub_gid_unlock();
			return false;
		}
		break;
	default:
		return false;
	}

	ret = new_subid_range(range, id_type, reuse);

	if (id_type == ID_TYPE_UID) {
		sub_uid_close();
		sub_uid_unlock();
	} else {
		sub_gid_close();
		sub_gid_unlock();
	}

	return ret;
}

bool grant_subuid_range(struct subordinate_range *range, bool reuse)
{
	return grant_subid_range(range, reuse, ID_TYPE_UID);
}

bool grant_subgid_range(struct subordinate_range *range, bool reuse)
{
	return grant_subid_range(range, reuse, ID_TYPE_GID);
}

bool free_subid_range(struct subordinate_range *range, enum subid_type id_type)
{
	bool ret;

	switch (id_type) {
	case ID_TYPE_UID:
		if (!sub_uid_lock()) {
			printf("Failed loging subuids (errno %d)\n", errno);
			return false;
		}
		if (!sub_uid_open(O_CREAT | O_RDWR)) {
			printf("Failed opening subuids (errno %d)\n", errno);
			sub_uid_unlock();
			return false;
		}
		break;
	case ID_TYPE_GID:
		if (!sub_gid_lock()) {
			printf("Failed loging subgids (errno %d)\n", errno);
			return false;
		}
		if (!sub_gid_open(O_CREAT | O_RDWR)) {
			printf("Failed opening subgids (errno %d)\n", errno);
			sub_gid_unlock();
			return false;
		}
		break;
	default:
		return false;
	}

	ret = release_subid_range(range, id_type);

	if (id_type == ID_TYPE_UID) {
		sub_uid_close();
		sub_uid_unlock();
	} else {
		sub_gid_close();
		sub_gid_unlock();
	}

	return ret;
}

bool free_subuid_range(struct subordinate_range *range)
{
	return free_subid_range(range, ID_TYPE_UID);
}

bool free_subgid_range(struct subordinate_range *range)
{
	return free_subid_range(range, ID_TYPE_GID);
}
