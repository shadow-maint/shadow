#include <config.h>

#include <assert.h>
#include <stdio.h>

#include "prototypes.h"
#include "pwio.h"
#include "groupio.h"
#include "getdef.h"

/*
 * find_new_uid - Find a new unused UID.
 *
 * If successful, find_new_uid provides an unused user ID in the
 * [UID_MIN:UID_MAX] range.
 * This ID should be higher than all the used UID, but if not possible,
 * the lowest unused ID in the range will be returned.
 * 
 * Return 0 on success, -1 if no unused UIDs are available.
 */
int find_new_uid (int sys_user, uid_t *uid, uid_t const *preferred_uid)
{
	const struct passwd *pwd;
	uid_t uid_min, uid_max, user_id;

	assert (uid != NULL);

	if (sys_user == 0) {
		uid_min = getdef_unum ("UID_MIN", 1000);
		uid_max = getdef_unum ("UID_MAX", 60000);
	} else {
		uid_min = getdef_unum ("SYS_UID_MIN", 1);
		uid_max = getdef_unum ("UID_MIN", 1000) - 1;
		uid_max = getdef_unum ("SYS_UID_MAX", uid_max);
	}

	if (   (NULL != preferred_uid)
	    && (*preferred_uid >= uid_min)
	    && (*preferred_uid <= uid_max)
	    /* Check if the user exists according to NSS */
	    && (getpwuid (*preferred_uid) == NULL)
	    /* Check also the local database in case of uncommitted
	     * changes */
	    && (pw_locate_uid (*preferred_uid) == NULL)) {
		*uid = *preferred_uid;
		return 0;
	}


	user_id = uid_min;

	/*
	 * Search the entire password file,
	 * looking for the largest unused value.
	 *
	 * We check the list of users according to NSS (setpwent/getpwent),
	 * but we also check the local database (pw_rewind/pw_next) in case
	 * some users were created but the changes were not committed yet.
	 */
	setpwent ();
	pw_rewind ();
	while (   ((pwd = getpwent ()) != NULL)
	       || ((pwd = pw_next ()) != NULL)) {
		if ((pwd->pw_uid >= user_id) && (pwd->pw_uid <= uid_max)) {
			user_id = pwd->pw_uid + 1;
		}
	}

	/*
	 * If a user with UID equal to UID_MAX exists, the above algorithm
	 * will give us UID_MAX+1 even if not unique. Search for the first
	 * free UID starting with UID_MIN (it's O(n*n) but can be avoided
	 * by not having users with UID equal to UID_MAX).  --marekm
	 */
	if (user_id == uid_max + 1) {
		for (user_id = uid_min; user_id < uid_max; user_id++) {
			/* local, no need for xgetpwuid */
			if (   (getpwuid (user_id) == NULL)
			    && (pw_locate_uid (user_id) == NULL)) {
				break;
			}
		}
		if (user_id == uid_max) {
			fputs (_("Can't get unique UID (no more available UIDs)\n"), stderr);
			return -1;
		}
	}

	*uid = user_id;
	return 0;
}

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
int find_new_gid (int sys_group, gid_t *gid, gid_t const *preferred_gid)
{
	const struct group *grp;
	gid_t gid_min, gid_max, group_id;

	assert (gid != NULL);

	if (sys_group == 0) {
		gid_min = getdef_unum ("GID_MIN", 1000);
		gid_max = getdef_unum ("GID_MAX", 60000);
	} else {
		gid_min = getdef_unum ("SYS_GID_MIN", 1);
		gid_max = getdef_unum ("GID_MIN", 1000) - 1;
		gid_max = getdef_unum ("SYS_GID_MAX", gid_max);
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

