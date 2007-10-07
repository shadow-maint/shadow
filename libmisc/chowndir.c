/*
 * Copyright 1992, 1993, Julianne Frances Haugh
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
 * 3. Neither the name of Julianne F. Haugh nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY JULIE HAUGH AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL JULIE HAUGH OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <config.h>

#include "rcsid.h"
RCSID ("$Id: chowndir.c,v 1.7 2003/04/22 10:59:21 kloczek Exp $")
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
 */
int
chown_tree (const char *root, uid_t old_uid, uid_t new_uid, gid_t old_gid,
	    gid_t new_gid)
{
	char new_name[1024];
	int rc = 0;
	struct DIRECT *ent;
	struct stat sb;
	DIR *dir;

	/*
	 * Make certain the directory exists.  This routine is called
	 * directory by the invoker, or recursively.
	 */

	if (access (root, F_OK) != 0)
		return -1;

	/*
	 * Open the directory and read each entry.  Every entry is tested
	 * to see if it is a directory, and if so this routine is called
	 * recursively.  If not, it is checked to see if it is owned by
	 * old user ID.
	 */

	if (!(dir = opendir (root)))
		return -1;

	while ((ent = readdir (dir))) {

		/*
		 * Skip the "." and ".." entries
		 */

		if (strcmp (ent->d_name, ".") == 0 ||
		    strcmp (ent->d_name, "..") == 0)
			continue;

		/*
		 * Make the filename for both the source and the
		 * destination files.
		 */

		if (strlen (root) + strlen (ent->d_name) + 2 >
		    sizeof new_name)
			break;

		snprintf (new_name, sizeof new_name, "%s/%s", root,
			  ent->d_name);

		/* Don't follow symbolic links! */
		if (LSTAT (new_name, &sb) == -1)
			continue;

		if (S_ISDIR (sb.st_mode) && !S_ISLNK (sb.st_mode)) {

			/*
			 * Do the entire subdirectory.
			 */

			if ((rc = chown_tree (new_name, old_uid, new_uid,
					      old_gid, new_gid)))
				break;
		}
#ifndef HAVE_LCHOWN
		/* don't use chown (follows symbolic links!) */
		if (S_ISLNK (sb.st_mode))
			continue;
#endif
		if (sb.st_uid == old_uid)
			LCHOWN (new_name, new_uid,
				sb.st_gid ==
				old_gid ? new_gid : sb.st_gid);
	}
	closedir (dir);

	/*
	 * Now do the root of the tree
	 */

	if (!stat (root, &sb)) {
		if (sb.st_uid == old_uid)
			LCHOWN (root, new_uid,
				sb.st_gid ==
				old_gid ? new_gid : sb.st_gid);
	}
	return rc;
}
