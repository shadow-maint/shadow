/*
 * Copyright (c) 1992 - 1993, Julianne Frances Haugh
 * Copyright (c) 1996 - 2000, Marek Michałkiewicz
 * Copyright (c) 2003 - 2005, Tomasz Kłoczko
 * Copyright (c) 2010 -     , Nicolas François
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

#ident "$Id$"

#include <sys/types.h>
#include <sys/stat.h>
#include "prototypes.h"
#include "defines.h"
#include <fcntl.h>
#include <stdio.h>
/*
 * chown_tree - change ownership of files in a directory tree
 *
 *	chown_dir() walks a directory tree and changes the ownership
 *	of all files owned by the provided user ID.
 *
 *	Only files owned (resp. group-owned) by old_uid (resp. by old_gid)
 *	will have their ownership (resp. group-ownership) modified, unless
 *	old_uid (resp. old_gid) is set to -1.
 *
 *	new_uid and new_gid can be set to -1 to indicate that no owner or
 *	group-owner shall be changed.
 */
int chown_tree (const char *root,
                uid_t old_uid,
                uid_t new_uid,
                gid_t old_gid,
                gid_t new_gid)
{
	char *new_name;
	size_t new_name_len;
	int rc = 0;
	struct DIRECT *ent;
	struct stat sb;
	DIR *dir;

	new_name = malloc (1024);
	if (NULL == new_name) {
		return -1;
	}
	new_name_len = 1024;

	/*
	 * Make certain the directory exists.  This routine is called
	 * directly by the invoker, or recursively.
	 */

	if (access (root, F_OK) != 0) {
		free (new_name);
		return -1;
	}

	/*
	 * Open the directory and read each entry.  Every entry is tested
	 * to see if it is a directory, and if so this routine is called
	 * recursively.  If not, it is checked to see if an ownership
	 * shall be changed.
	 */

	dir = opendir (root);
	if (NULL == dir) {
		free (new_name);
		return -1;
	}

	while ((ent = readdir (dir))) {
		size_t ent_name_len;
		uid_t tmpuid = (uid_t) -1;
		gid_t tmpgid = (gid_t) -1;

		/*
		 * Skip the "." and ".." entries
		 */

		if (   (strcmp (ent->d_name, ".") == 0)
		    || (strcmp (ent->d_name, "..") == 0)) {
			continue;
		}

		/*
		 * Make the filename for both the source and the
		 * destination files.
		 */

		ent_name_len = strlen (root) + strlen (ent->d_name) + 2;
		if (ent_name_len > new_name_len) {
			char *tmp = realloc (new_name, ent_name_len);
			if (NULL == tmp) {
				rc = -1;
				break;
			}
			new_name = tmp;
			new_name_len = ent_name_len;
		}

		(void) snprintf (new_name, new_name_len, "%s/%s", root, ent->d_name);

		/* Don't follow symbolic links! */
		if (LSTAT (new_name, &sb) == -1) {
			continue;
		}

		if (S_ISDIR (sb.st_mode) && !S_ISLNK (sb.st_mode)) {

			/*
			 * Do the entire subdirectory.
			 */

			rc = chown_tree (new_name, old_uid, new_uid,
			                 old_gid, new_gid);
			if (0 != rc) {
				break;
			}
		}
#ifndef HAVE_LCHOWN
		/* don't use chown (follows symbolic links!) */
		if (S_ISLNK (sb.st_mode)) {
			continue;
		}
#endif
		/*
		 * By default, the IDs are not changed (-1).
		 *
		 * If the file is not owned by the user, the owner is not
		 * changed.
		 *
		 * If the file is not group-owned by the group, the
		 * group-owner is not changed.
		 */
		if (((uid_t) -1 == old_uid) || (sb.st_uid == old_uid)) {
			tmpuid = new_uid;
		}
		if (((gid_t) -1 == old_gid) || (sb.st_gid == old_gid)) {
			tmpgid = new_gid;
		}
		if (((uid_t) -1 != tmpuid) || ((gid_t) -1 != tmpgid)) {
			rc = LCHOWN (new_name, tmpuid, tmpgid);
			if (0 != rc) {
				break;
			}
		}
	}

	free (new_name);
	(void) closedir (dir);

	/*
	 * Now do the root of the tree
	 */

	if ((0 == rc) && (stat (root, &sb) == 0)) {
		uid_t tmpuid = (uid_t) -1;
		gid_t tmpgid = (gid_t) -1;
		if (((uid_t) -1 == old_uid) || (sb.st_uid == old_uid)) {
			tmpuid = new_uid;
		}
		if (((gid_t) -1 == old_gid) || (sb.st_gid == old_gid)) {
			tmpgid = new_gid;
		}
		if (((uid_t) -1 != tmpuid) || ((gid_t) -1 != tmpgid)) {
			rc = LCHOWN (root, tmpuid, tmpgid);
		}
	} else {
		rc = -1;
	}

	return rc;
}

