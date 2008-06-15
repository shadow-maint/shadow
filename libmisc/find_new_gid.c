/*
 * Copyright (c) 1991 - 1994, Julianne Frances Haugh
 * Copyright (c) 2008       , Nicolas François
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

#include <assert.h>
#include <stdio.h>

#include "prototypes.h"
#include "groupio.h"
#include "getdef.h"

/*
 * find_new_gid - Find a new unused GID.
 *
 * If successful, find_new_gid provides an unused group ID in the
 * [GID_MIN:GID_MAX] range.
 * This ID should be higher than all the used GID, but if not possible,
 * the lowest unused ID in the range will be returned.
 * 
 * Return 0 on success, -1 if no unused GIDs are available.
 */
int find_new_gid (bool sys_group, gid_t *gid, gid_t const *preferred_gid)
{
	const struct group *grp;
	gid_t gid_min, gid_max, group_id;

	assert (gid != NULL);

	if (!sys_group) {
		gid_min = getdef_ulong ("GID_MIN", 1000L);
		gid_max = getdef_ulong ("GID_MAX", 60000L);
	} else {
		gid_min = getdef_ulong ("SYS_GID_MIN", 1L);
		gid_max = getdef_ulong ("GID_MIN", 1000L) - 1;
		gid_max = getdef_ulong ("SYS_GID_MAX", (unsigned long) gid_max);
	}

	if (   (NULL != preferred_gid)
	    && (*preferred_gid >= gid_min)
	    && (*preferred_gid <= gid_max)
	    /* Check if the user exists according to NSS */
	    && (getgrgid (*preferred_gid) == NULL)
	    /* Check also the local database in case of uncommitted
	     * changes */
	    && (gr_locate_gid (*preferred_gid) == NULL)) {
		*gid = *preferred_gid;
		return 0;
	}

	group_id = gid_min;

	/*
	 * Search the entire group file,
	 * looking for the largest unused value.
	 *
	 * We check the list of users according to NSS (setpwent/getpwent),
	 * but we also check the local database (pw_rewind/pw_next) in case
	 * some groups were created but the changes were not committed yet.
	 */
	setgrent ();
	gr_rewind ();
	while (   ((grp = getgrent ()) != NULL)
	       || ((grp = gr_next ()) != NULL)) {
		if ((grp->gr_gid >= group_id) && (grp->gr_gid <= gid_max)) {
			group_id = grp->gr_gid + 1;
		}
	}
	endgrent ();

	/*
	 * If a group with GID equal to GID_MAX exists, the above algorithm
	 * will give us GID_MAX+1 even if not unique. Search for the first
	 * free GID starting with GID_MIN (it's O(n*n) but can be avoided
	 * by not having users with GID equal to GID_MAX).  --marekm
	 */
	if (group_id == gid_max + 1) {
		for (group_id = gid_min; group_id < gid_max; group_id++) {
			/* local, no need for xgetgrgid */
			if (   (getgrgid (group_id) == NULL)
			    && (gr_locate_gid (group_id) == NULL)) {
				break;
			}
		}
		if (group_id == gid_max) {
			fputs (_("Can't get unique GID (no more available GIDs)\n"), stderr);
			return -1;
		}
	}

	*gid = group_id;
	return 0;
}

