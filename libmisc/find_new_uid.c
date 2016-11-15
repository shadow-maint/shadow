/*
 * Copyright (c) 1991 - 1994, Julianne Frances Haugh
 * Copyright (c) 2008 - 2011, Nicolas François
 * Copyright (c) 2014, Red Hat, Inc.
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
#include <errno.h>

#include "prototypes.h"
#include "pwio.h"
#include "getdef.h"

/*
 * get_ranges - Get the minimum and maximum ID ranges for the search
 *
 * This function will return the minimum and maximum ranges for IDs
 *
 * 0: The function completed successfully
 * EINVAL: The provided ranges are impossible (such as maximum < minimum)
 *
 * preferred_min: The special-case minimum value for a specifically-
 * requested ID, which may be lower than the standard min_id
 */
static int get_ranges (bool sys_user, uid_t *min_id, uid_t *max_id,
			uid_t *preferred_min)
{
	uid_t uid_def_max = 0;

	if (sys_user) {
		/* System users */

		/* A requested ID is allowed to be below the autoselect range */
		*preferred_min = (uid_t) 1;

		/* Get the minimum ID range from login.defs or default to 101 */
		*min_id = (uid_t) getdef_ulong ("SYS_UID_MIN", 101UL);

		/*
		 * If SYS_UID_MAX is unspecified, we should assume it to be one
		 * less than the UID_MIN (which is reserved for non-system accounts)
		 */
		uid_def_max = (uid_t) getdef_ulong ("UID_MIN", 1000UL) - 1;
		*max_id = (uid_t) getdef_ulong ("SYS_UID_MAX",
				(unsigned long) uid_def_max);

		/* Check that the ranges make sense */
		if (*max_id < *min_id) {
			(void) fprintf (stderr,
                            _("%s: Invalid configuration: SYS_UID_MIN (%lu), "
                              "UID_MIN (%lu), SYS_UID_MAX (%lu)\n"),
                            Prog, (unsigned long) *min_id,
                            getdef_ulong ("UID_MIN", 1000UL),
                            (unsigned long) *max_id);
			return EINVAL;
		}
	} else {
		/* Non-system users */

		/* Get the values from login.defs or use reasonable defaults */
		*min_id = (uid_t) getdef_ulong ("UID_MIN", 1000UL);
		*max_id = (uid_t) getdef_ulong ("UID_MAX", 60000UL);

		/*
		 * The preferred minimum should match the standard ID minimum
		 * for non-system users.
		 */
		*preferred_min = *min_id;

		/* Check that the ranges make sense */
		if (*max_id < *min_id) {
			(void) fprintf (stderr,
					_("%s: Invalid configuration: UID_MIN (%lu), "
					  "UID_MAX (%lu)\n"),
					Prog, (unsigned long) *min_id,
					(unsigned long) *max_id);
			return EINVAL;
		}
	}

	return 0;
}

/*
 * check_uid - See if the requested UID is available
 *
 * On success, return 0
 * If the ID is in use, return EEXIST
 * If the ID is outside the range, return ERANGE
 * In other cases, return errno from getpwuid()
 */
static int check_uid(const uid_t uid,
		     const uid_t uid_min,
		     const uid_t uid_max,
		     bool *used_uids)
{
	/* First test that the preferred ID is in the range */
	if (uid < uid_min || uid > uid_max) {
		return ERANGE;
	}

	/*
	 * Check whether we already detected this UID
	 * using the pw_next() loop
	 */
	if (used_uids != NULL && used_uids[uid]) {
		return EEXIST;
	}
	/* Check if the UID exists according to NSS */
	errno = 0;
	if (getpwuid(uid) != NULL) {
		return EEXIST;
	} else {
		/* getpwuid() was NULL
		 * we have to ignore errors as temporary
		 * failures of remote user identity services
		 * would completely block user/group creation
		 */
	}

	/* If we've made it here, the UID must be available */
	return 0;
}

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
int find_new_uid(bool sys_user,
                 uid_t *uid,
                 /*@null@*/uid_t const *preferred_uid)
{
	bool *used_uids;
	const struct passwd *pwd;
	uid_t uid_min, uid_max, preferred_min;
	uid_t user_id, id;
	uid_t lowest_found, highest_found;
	int result;
	int nospam = 0;

	assert (uid != NULL);

	/*
	 * First, figure out what ID range is appropriate for
	 * automatic assignment
	 */
	result = get_ranges (sys_user, &uid_min, &uid_max, &preferred_min);
	if (result == EINVAL) {
		return -1;
	}

	/* Check if the preferred UID is available */
	if (preferred_uid) {
		result = check_uid (*preferred_uid, preferred_min, uid_max, NULL);
		if (result == 0) {
			/*
			 * Make sure the UID isn't queued for use already
			 */
			if (pw_locate_uid (*preferred_uid) == NULL) {
				*uid = *preferred_uid;
				return 0;
			}
			/*
			 * pw_locate_uid() found the UID in an as-yet uncommitted
			 * entry. We'll proceed below and auto-set an UID.
			 */
		} else if (result == EEXIST || result == ERANGE) {
			/*
			 * Continue on below. At this time, we won't
			 * treat these two cases differently.
			 */
		} else {
			/*
			 * An unexpected error occurred. We should report
			 * this and fail the user creation.
			 * This differs from the automatic creation
			 * behavior below, since if a specific UID was
			 * requested and generated an error, the user is
			 * more likely to want to stop and address the
			 * issue.
			 */
			fprintf (stderr,
				_("%s: Encountered error attempting to use "
				  "preferred UID: %s\n"),
				Prog, strerror (result));
			return -1;
		}
	}

	/*
	 * Search the entire passwd file,
	 * looking for the next unused value.
	 *
	 * We first check the local database with pw_rewind/pw_next to find
	 * all local values that are in use.
	 *
	 * We then compare the next free value to all databases (local and
	 * remote) and iterate until we find a free one. If there are free
	 * values beyond the lowest (system users) or highest (non-system
	 * users), we will prefer those and avoid potentially reclaiming a
	 * deleted user (which can be a security issue, since it may grant
	 * access to files belonging to that former user).
	 *
	 * If there are no UIDs available at the end of the search, we will
	 * have no choice but to iterate through the range looking for gaps.
	 *
	 */

	/* Create an array to hold all of the discovered UIDs */
	used_uids = malloc (sizeof (bool) * (uid_max +1));
	if (NULL == used_uids) {
		fprintf (stderr,
			 _("%s: failed to allocate memory: %s\n"),
			 Prog, strerror (errno));
		return -1;
	}
	memset (used_uids, false, sizeof (bool) * (uid_max + 1));

	/* First look for the lowest and highest value in the local database */
	(void) pw_rewind ();
	highest_found = uid_min;
	lowest_found = uid_max;
	while ((pwd = pw_next ()) != NULL) {
		/*
		 * Does this entry have a lower UID than the lowest we've found
		 * so far?
		 */
		if ((pwd->pw_uid <= lowest_found) && (pwd->pw_uid >= uid_min)) {
			lowest_found = pwd->pw_uid - 1;
		}

		/*
		 * Does this entry have a higher UID than the highest we've found
		 * so far?
		 */
		if ((pwd->pw_uid >= highest_found) && (pwd->pw_uid <= uid_max)) {
			highest_found = pwd->pw_uid + 1;
		}

		/* create index of used UIDs */
		if (pwd->pw_uid >= uid_min
			&& pwd->pw_uid <= uid_max) {

			used_uids[pwd->pw_uid] = true;
		}
	}

	if (sys_user) {
		/*
		 * For system users, we want to start from the
		 * top of the range and work downwards.
		 */

		/*
		 * At the conclusion of the pw_next() search, we will either
		 * have a presumed-free UID or we will be at UID_MIN - 1.
		 */
		if (lowest_found < uid_min) {
			/*
			 * In this case, an UID is in use at UID_MIN.
			 *
			 * We will reset the search to UID_MAX and proceed down
			 * through all the UIDs (skipping those we detected with
			 * used_uids) for a free one. It is a known issue that
			 * this may result in reusing a previously-deleted UID,
			 * so administrators should be instructed to use this
			 * auto-detection with care (and prefer to assign UIDs
			 * explicitly).
			 */
			lowest_found = uid_max;
		}

		/* Search through all of the IDs in the range */
		for (id = lowest_found; id >= uid_min; id--) {
			result = check_uid (id, uid_min, uid_max, used_uids);
			if (result == 0) {
				/* This UID is available. Return it. */
				*uid = id;
				free (used_uids);
				return 0;
			} else if (result == EEXIST) {
				/* This UID is in use, we'll continue to the next */
			} else {
				/*
				 * An unexpected error occurred.
				 *
				 * Only report it the first time to avoid spamming
				 * the logs
				 *
				 */
				if (!nospam) {
					fprintf (stderr,
						_("%s: Can't get unique system UID (%s). "
						  "Suppressing additional messages.\n"),
						Prog, strerror (result));
					SYSLOG ((LOG_ERR,
						"Error checking available UIDs: %s",
						strerror (result)));
					nospam = 1;
				}
				/*
				 * We will continue anyway. Hopefully a later UID
				 * will work properly.
				 */
			}
		}

		/*
		 * If we get all the way through the loop, try again from UID_MAX,
		 * unless that was where we previously started. (NOTE: the worst-case
		 * scenario here is that we will run through (UID_MAX - UID_MIN - 1)
		 * cycles *again* if we fall into this case with lowest_found as
		 * UID_MAX - 1, all users in the range in use and maintained by
		 * network services such as LDAP.)
		 */
		if (lowest_found != uid_max) {
			for (id = uid_max; id >= uid_min; id--) {
				result = check_uid (id, uid_min, uid_max, used_uids);
				if (result == 0) {
					/* This UID is available. Return it. */
					*uid = id;
					free (used_uids);
					return 0;
				} else if (result == EEXIST) {
					/* This UID is in use, we'll continue to the next */
				} else {
					/*
					 * An unexpected error occurred.
					 *
					 * Only report it the first time to avoid spamming
					 * the logs
					 *
					 */
					if (!nospam) {
						fprintf (stderr,
							_("%s: Can't get unique system UID (%s). "
							  "Suppressing additional messages.\n"),
							Prog, strerror (result));
						SYSLOG((LOG_ERR,
							"Error checking available UIDs: %s",
							strerror (result)));
						nospam = 1;
					}
					/*
					 * We will continue anyway. Hopefully a later UID
					 * will work properly.
					 */
				}
			}
		}
	} else { /* !sys_user */
		/*
		 * For non-system users, we want to start from the
		 * bottom of the range and work upwards.
		 */

		/*
		 * At the conclusion of the pw_next() search, we will either
		 * have a presumed-free UID or we will be at UID_MAX + 1.
		 */
		if (highest_found > uid_max) {
			/*
			 * In this case, a UID is in use at UID_MAX.
			 *
			 * We will reset the search to UID_MIN and proceed up
			 * through all the UIDs (skipping those we detected with
			 * used_uids) for a free one. It is a known issue that
			 * this may result in reusing a previously-deleted UID,
			 * so administrators should be instructed to use this
			 * auto-detection with care (and prefer to assign UIDs
			 * explicitly).
			 */
			highest_found = uid_min;
		}

		/* Search through all of the IDs in the range */
		for (id = highest_found; id <= uid_max; id++) {
			result = check_uid (id, uid_min, uid_max, used_uids);
			if (result == 0) {
				/* This UID is available. Return it. */
				*uid = id;
				free (used_uids);
				return 0;
			} else if (result == EEXIST) {
				/* This UID is in use, we'll continue to the next */
			} else {
				/*
				 * An unexpected error occurred.
				 *
				 * Only report it the first time to avoid spamming
				 * the logs
				 *
				 */
				if (!nospam) {
					fprintf (stderr,
						_("%s: Can't get unique UID (%s). "
						  "Suppressing additional messages.\n"),
						Prog, strerror (result));
					SYSLOG ((LOG_ERR,
						"Error checking available UIDs: %s",
						strerror (result)));
					nospam = 1;
				}
				/*
				 * We will continue anyway. Hopefully a later UID
				 * will work properly.
				 */
			}
		}

		/*
		 * If we get all the way through the loop, try again from UID_MIN,
		 * unless that was where we previously started. (NOTE: the worst-case
		 * scenario here is that we will run through (UID_MAX - UID_MIN - 1)
		 * cycles *again* if we fall into this case with highest_found as
		 * UID_MIN + 1, all users in the range in use and maintained by
		 * network services such as LDAP.)
		 */
		if (highest_found != uid_min) {
			for (id = uid_min; id <= uid_max; id++) {
				result = check_uid (id, uid_min, uid_max, used_uids);
				if (result == 0) {
					/* This UID is available. Return it. */
					*uid = id;
					free (used_uids);
					return 0;
				} else if (result == EEXIST) {
					/* This UID is in use, we'll continue to the next */
				} else {
					/*
					 * An unexpected error occurred.
					 *
					 * Only report it the first time to avoid spamming
					 * the logs
					 *
					 */
					if (!nospam) {
						fprintf (stderr,
							_("%s: Can't get unique UID (%s). "
							  "Suppressing additional messages.\n"),
							Prog, strerror (result));
						SYSLOG ((LOG_ERR,
							"Error checking available UIDs: %s",
							strerror (result)));
						nospam = 1;
					}
					/*
					 * We will continue anyway. Hopefully a later UID
					 * will work properly.
					 */
				}
			}
		}
	}

	/* The code reached here and found no available IDs in the range */
	fprintf (stderr,
		_("%s: Can't get unique UID (no more available UIDs)\n"),
		Prog);
	SYSLOG ((LOG_WARN, "no more available UIDs on the system"));
	free (used_uids);
	return -1;
}

