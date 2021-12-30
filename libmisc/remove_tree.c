/*
 * SPDX-FileCopyrightText: 1991 - 1994, Julianne Frances Haugh
 * SPDX-FileCopyrightText: 1996 - 2001, Marek Michałkiewicz
 * SPDX-FileCopyrightText: 2003 - 2006, Tomasz Kłoczko
 * SPDX-FileCopyrightText: 2007 - 2010, Nicolas François
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <config.h>

#ident "$Id$"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include "prototypes.h"
#include "defines.h"

/*
 * remove_tree - delete a directory tree
 *
 *	remove_tree() walks a directory tree and deletes all the files
 *	and directories.
 *	At the end, it deletes the root directory itself.
 */

int remove_tree (const char *root, bool remove_root)
{
	char *new_name = NULL;
	int err = 0;
	struct dirent *ent;
	struct stat sb;
	DIR *dir;

	/*
	 * Open the source directory and read each entry.  Every file
	 * entry in the directory is copied with the UID and GID set
	 * to the provided values.  As an added security feature only
	 * regular files (and directories ...) are copied, and no file
	 * is made set-ID.
	 */
	dir = opendir (root);
	if (NULL == dir) {
		return -1;
	}

	while ((ent = readdir (dir))) {
		size_t new_len = strlen (root) + strlen (ent->d_name) + 2;

		/*
		 * Skip the "." and ".." entries
		 */

		if (strcmp (ent->d_name, ".") == 0 ||
		    strcmp (ent->d_name, "..") == 0) {
			continue;
		}

		/*
		 * Make the filename for the current entry.
		 */

		free (new_name);
		new_name = (char *) malloc (new_len);
		if (NULL == new_name) {
			err = -1;
			break;
		}
		(void) snprintf (new_name, new_len, "%s/%s", root, ent->d_name);
		if (LSTAT (new_name, &sb) == -1) {
			continue;
		}

		if (S_ISDIR (sb.st_mode)) {
			/*
			 * Recursively delete this directory.
			 */
			if (remove_tree (new_name, true) != 0) {
				err = -1;
				break;
			}
		} else {
			/*
			 * Delete the file.
			 */
			if (unlink (new_name) != 0) {
				err = -1;
				break;
			}
		}
	}
	if (NULL != new_name) {
		free (new_name);
	}
	(void) closedir (dir);

	if (remove_root && (0 == err)) {
		if (rmdir (root) != 0) {
			err = -1;
		}
	}

	return err;
}

