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
#include "pwio.h"
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
int find_new_uid (bool sys_user,
                  uid_t *uid,
                  /*@null@*/uid_t const *preferred_uid)
{
	const struct passwd *pwd;
	uid_t uid_min, uid_max, user_id;
	bool *used_uids;

	assert (uid != NULL);

	if (!sys_user) {
		uid_min = (uid_t) getdef_ulong ("UID_MIN", 1000UL);
		uid_max = (uid_t) getdef_ulong ("UID_MAX", 60000UL);
		if (uid_max < uid_min) {
			(void) fprintf (stderr,
			                _("%s: Invalid configuration: UID_MIN (%lu), UID_MAX (%lu)\n"),
			                Prog, (unsigned long) uid_min, (unsigned long) uid_max);
		}
	} else {
		uid_min = (uid_t) getdef_ulong ("SYS_UID_MIN", 101UL);
		uid_max = (uid_t) getdef_ulong ("UID_MIN", 1000UL) - 1;
		uid_max = (uid_t) getdef_ulong ("SYS_UID_MAX", (unsigned long) uid_max);
		if (uid_max < uid_min) {
			(void) fprintf (stderr,
			                _("%s: Invalid configuration: SYS_UID_MIN (%lu), UID_MIN (%lu), SYS_UID_MAX (%lu)\n"),
			                Prog, (unsigned long) uid_min, getdef_ulong ("UID_MIN", 1000UL), (unsigned long) uid_max);
		}
	}
	used_uids = alloca (sizeof (bool) * (uid_max +1));
	memset (used_uids, false, sizeof (bool) * (uid_max + 1));

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


	/*
	 * Search the entire password file,
	 * looking for the largest unused value.
	 *
	 * We check the list of users according to NSS (setpwent/getpwent),
	 * but we also check the local database (pw_rewind/pw_next) in case
	 * some users were created but the changes were not committed yet.
	 */
	if (sys_user) {
		uid_t id;
		/* setpwent / getpwent / endpwent can be very slow with
		 * LDAP configurations (and many accounts).
		 * Since there is a limited amount of IDs to be tested
		 * for system accounts, we just check the existence
		 * of IDs with getpwuid.
		 */
		user_id = uid_max;
		for (id = uid_max; id >= uid_min; id--) {
			if (getpwuid (id) != NULL) {
				user_id = id - 1;
				used_uids[id] = true;
			}
		}

		pw_rewind ();
		while ((pwd = pw_next ()) != NULL) {
			if ((pwd->pw_uid <= user_id) && (pwd->pw_uid >= uid_min)) {
				user_id = pwd->pw_uid - 1;
			}
			/* create index of used UIDs */
			if (pwd->pw_uid <= uid_max) {
				used_uids[pwd->pw_uid] = true;
			}
		}
	} else {
		user_id = uid_min;
		setpwent ();
		while ((pwd = getpwent ()) != NULL) {
			if ((pwd->pw_uid >= user_id) && (pwd->pw_uid <= uid_max)) {
				user_id = pwd->pw_uid + 1;
			}
			/* create index of used UIDs */
			if (pwd->pw_uid <= uid_max) {
				used_uids[pwd->pw_uid] = true;
			}
		}
		endpwent ();

		pw_rewind ();
		while ((pwd = pw_next ()) != NULL) {
			if ((pwd->pw_uid >= user_id) && (pwd->pw_uid <= uid_max)) {
				user_id = pwd->pw_uid + 1;
			}
			/* create index of used UIDs */
			if (pwd->pw_uid <= uid_max) {
				used_uids[pwd->pw_uid] = true;
			}
		}
	}

	/*
	 * If a user with UID equal to UID_MAX exists, the above algorithm
	 * will give us UID_MAX+1 even if not unique. Search for the first
	 * free UID starting with UID_MIN.
	 */
	if (sys_user) {
		if (user_id == uid_min - 1) {
			for (user_id = uid_max; user_id >= uid_min; user_id--) {
				if (false == used_uids[user_id]) {
					break;
				}
			}
			if (user_id < uid_min ) {
				fprintf (stderr,
				         _("%s: Can't get unique system UID (no more available UIDs)\n"),
				         Prog);
				SYSLOG ((LOG_WARN,
				         "no more available UID on the system"));
				return -1;
			}
		}
	} else {
		if (user_id == uid_max + 1) {
			for (user_id = uid_min; user_id < uid_max; user_id++) {
				if (false == used_uids[user_id]) {
					break;
				}
			}
			if (user_id == uid_max) {
				fprintf (stderr,
				         _("%s: Can't get unique UID (no more available UIDs)\n"),
				         Prog);
				SYSLOG ((LOG_WARN, "no more available UID on the system"));
				return -1;
			}
		}
	}

	*uid = user_id;
	return 0;
}

