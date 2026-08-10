/*
 * Copyright 1992, 1993, John F. Haugh II
 * All rights reserved.
 *
 * Permission is granted to copy and create derivative works for any
 * non-commercial purpose, provided this copyright notice is preserved
 * in all copies of source code, or included in human readable form
 * and conspicuously displayed on all copies of object code or
 * distribution media.
 *
 * This software is provided on an AS-IS basis and the author makes
 * no warrantee of any kind.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include "config.h"

#if defined(DIR_XENIX) || defined(DIR_BSD) || defined(DIR_SYSV)

#ifdef DIR_XENIX
#include <sys/ndir.h>
#define DIRECT direct
#endif
#ifdef DIR_BSD
#include <ndir.h>
#define DIRECT direct
#endif
#ifdef DIR_SYSV
#include <dirent.h>
#define DIRECT dirent
#endif
#include <fcntl.h>
#include <stdio.h>

#ifndef lint
static	char	sccsid[] = "@(#)chowndir.c	3.2	08:00:46	08 Apr 1993";
#endif

#ifndef	S_ISDIR
#define	S_ISDIR(x)	(((x)&S_IFMT)==S_IFDIR)
#endif
#ifndef	S_ISREG
#define	S_ISREG(x)	(((x)&S_IFMT)==S_IFREG)
#endif

/*
 * chown_tree - change ownership of files in a directory tree
 *
 *	chown_dir() walks a directory tree and changes the ownership
 *	of all files owned by the provided user ID.
 */

int
chown_tree (root, old_uid, new_uid, old_gid, new_gid)
char	*root;
int	old_uid, new_uid;
int	old_gid, new_gid;
{
	char	new_name[BUFSIZ];
	int	cnt;
	int	rc = 0;
	struct	DIRECT	*ent;
	struct	stat	sb;
	DIR	*dir;

	/*
	 * Make certain the directory exists.  This routine is called
	 * directory by the invoker, or recursively.
	 */

	if (access (root, 0) != 0)
		return -1;

	/*
	 * Open the directory and read each entry.  Every entry is tested
	 * to see if it is a directory, and if so this routine is called
	 * recursively.  If not, it is checked to see if it is owned by
	 * old user ID.
	 */

	if (! (dir = opendir (root)))
		return -1;

	while (ent = readdir (dir)) {

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

		if (strlen (root) + strlen (ent->d_name) + 2 > BUFSIZ)
			break;

		sprintf (new_name, "%s/%s", root, ent->d_name);

		if (stat (new_name, &sb) == -1)
			continue;

		if (S_ISDIR (sb.st_mode)) {

			/*
			 * Do the entire subdirectory.
			 */

			if (rc = chown_tree (new_name, old_uid, new_uid,
					old_gid, new_gid))
				break;
		}
		if (sb.st_uid == old_uid)
			chown (new_name, new_uid,
				sb.st_gid == old_gid ? new_gid:sb.st_gid);
	}
	closedir (dir);

	/*
	 * Now do the root of the tree
	 */

	if (! stat (root, &sb)) {
		if (sb.st_uid == old_uid)
			chown (root, new_uid,
				sb.st_gid == old_gid ? new_gid:sb.st_gid);
	}
	return rc;
}
#endif	/* defined(DIR_XXX) */
