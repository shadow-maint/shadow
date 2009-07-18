/*
 * Copyright (c) 1991 - 1994, Julianne Frances Haugh
 * Copyright (c) 2008 - 2009, Nicolas François
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
int find_new_gid (bool sys_group,
                  gid_t *gid,
                  /*@null@*/gid_t const *preferred_gid)
{
	const struct group *grp;
	gid_t gid_min, gid_max, group_id, id;
	bool *used_gids;

	assert (gid != NULL);

	if (!sys_group) {
		gid_min = (gid_t) getdef_ulong ("GID_MIN", 1000UL);
		gid_max = (gid_t) getdef_ulong ("GID_MAX", 60000UL);
	} else {
		gid_min = (gid_t) getdef_ulong ("SYS_GID_MIN", 101UL);
		gid_max = (gid_t) getdef_ulong ("GID_MIN", 1000UL) - 1;
		gid_max = (gid_t) getdef_ulong ("SYS_GID_MAX", (unsigned long) gid_max);
	}
	used_gids = alloca (sizeof (bool) * (gid_max +1));
	memset (used_gids, false, sizeof (bool) * (gid_max + 1));

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


	/*
	 * Search the entire group file,
	 * looking for the largest unused value.
	 *
	 * We check the list of groups according to NSS (setgrent/getgrent),
	 * but we also check the local database (gr_rewind/gr_next) in case
	 * some groups were created but the changes were not committed yet.
	 */
	if (sys_group) {
		/* setgrent / getgrent / endgrent can be very slow with
		 * LDAP configurations (and many accounts).
		 * Since there is a limited amount of IDs to be tested
		 * for system accounts, we just check the existence
		 * of IDs with getgrgid.
		 */
		group_id = gid_max;
		for (id = gid_max; id >= gid_min; id--) {
			if (getgrgid (id) != NULL) {
				group_id = id - 1;
				used_gids[id] = true;
			}
		}

		gr_rewind ();
		while ((grp = gr_next ()) != NULL) {
			if ((grp->gr_gid <= group_id) && (grp->gr_gid >= gid_min)) {
				group_id = grp->gr_gid - 1;
			}
			/* create index of used GIDs */
			if (grp->gr_gid <= gid_max) {
				used_gids[grp->gr_gid] = true;
			}
		}
	} else {
		group_id = gid_min;
		setgrent ();
		while ((grp = getgrent ()) != NULL) {
			if ((grp->gr_gid >= group_id) && (grp->gr_gid <= gid_max)) {
				group_id = grp->gr_gid + 1;
			}
			/* create index of used GIDs */
			if (grp->gr_gid <= gid_max) {
				used_gids[grp->gr_gid] = true;
			}
		}
		endgrent ();

		gr_rewind ();
		while ((grp = gr_next ()) != NULL) {
			if ((grp->gr_gid >= group_id) && (grp->gr_gid <= gid_max)) {
				group_id = grp->gr_gid + 1;
			}
			/* create index of used GIDs */
			if (grp->gr_gid <= gid_max) {
				used_gids[grp->gr_gid] = true;
			}
		}
	}

	/*
	 * If a group with GID equal to GID_MAX exists, the above algorithm
	 * will give us GID_MAX+1 even if not unique. Search for the first
	 * free GID starting with GID_MIN.
	 */
	if (sys_group) {
		if (group_id == gid_min - 1) {
			for (group_id = gid_max; group_id >= gid_min; group_id--) {
				if (false == used_gids[group_id]) {
					break;
				}
			}
			if ( group_id < gid_min ) {
				fprintf (stderr,
				         _("%s: Can't get unique system GID (no more available GIDs)\n"),
				         Prog);
				SYSLOG ((LOG_WARN,
				         "no more available GID on the system"));
				return -1;
			}
		}
	} else {
		if (group_id == gid_max + 1) {
			for (group_id = gid_min; group_id < gid_max; group_id++) {
				if (false == used_gids[group_id]) {
					break;
				}
			}
			if (group_id == gid_max) {
				fprintf (stderr,
				         _("%s: Can't get unique GID (no more available GIDs)\n"),
				         Prog);
				SYSLOG ((LOG_WARN, "no more available GID on the system"));
				return -1;
			}
		}
	}

	*gid = group_id;
	return 0;
}

