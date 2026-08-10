/*
 * Copyright 1991, 1992, 1993, John F. Haugh II
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
static	char	sccsid[] = "@(#)copydir.c	3.3	07:59:20	20 Apr 1993";
#endif

#ifndef	S_ISDIR
#define	S_ISDIR(x)	(((x)&S_IFMT)==S_IFDIR)
#endif
#ifndef	S_ISREG
#define	S_ISREG(x)	(((x)&S_IFMT)==S_IFREG)
#endif

static	char	*src_orig;
static	char	*dst_orig;

struct	link_name {
	int	ln_dev;
	int	ln_ino;
	int	ln_count;
	char	*ln_name;
	struct	link_name *ln_next;
};
static	struct	link_name *links;

/*
 * remove_link - delete a link from the link list
 */

void
remove_link (link)
struct	link_name *link;
{
	struct link_name *lp;

	if (links == link) {
		links = link->ln_next;
		free (link->ln_name);
		free (link);
		return;
	}
	for (lp = links;lp;lp = lp->ln_next)
		if (lp->ln_next == link)
			break;

	if (! lp)
		return;

	lp->ln_next = lp->ln_next->ln_next;
	free (link->ln_name);
	free (link);
}

/*
 * check_link - see if a file is really a link
 */

struct link_name *
check_link (name, sb)
char	*name;
struct	stat	*sb;
{
	struct	link_name *lp;
	int	src_len;
	int	dst_len;
	int	name_len;
	char	*malloc ();

	for (lp = links;lp;lp = lp->ln_next)
		if (lp->ln_dev == sb->st_dev && lp->ln_ino == sb->st_ino)
			return lp;

	if (sb->st_nlink == 1)
		return 0;

	lp = (struct link_name *) malloc (sizeof *lp);
	src_len = strlen (src_orig);
	dst_len = strlen (dst_orig);
	name_len = strlen (name);
	lp->ln_dev = sb->st_dev;
	lp->ln_ino = sb->st_ino;
	lp->ln_count = sb->st_nlink;
	lp->ln_name = malloc (name_len - src_len + dst_len + 1);
	sprintf (lp->ln_name, "%s%s", dst_orig, name + src_len);
	lp->ln_next = links;
	links = lp;

	return 0;
}

/*
 * copy_tree - copy files in a directory tree
 *
 *	copy_tree() walks a directory tree and copies ordinary files
 *	as it goes.
 */

int
copy_tree (src_root, dst_root, uid, gid, ouid, ogid)
char	*src_root;
char	*dst_root;
UID_T	uid;
GID_T	gid;
UID_T	ouid;
GID_T	ogid;
{
	char	src_name[BUFSIZ];
	char	dst_name[BUFSIZ];
	char	buf[BUFSIZ];
	int	ifd;
	int	ofd;
	int	err = 0;
	int	cnt;
	int	set_orig = 0;
	struct	DIRECT	*ent;
	struct	stat	sb;
	struct	link_name *lp;
	DIR	*dir;

	/*
	 * Make certain both directories exist.  This routine is called
	 * after the home directory is created, or recursively after the
	 * target is created.  It assumes the target directory exists.
	 */

	if (access (src_root, 0) != 0 || access (dst_root, 0) != 0)
		return -1;

	/*
	 * Open the source directory and read each entry.  Every file
	 * entry in the directory is copied with the UID and GID set
	 * to the provided values.  As an added security feature only
	 * regular files (and directories ...) are copied, and no file
	 * is made set-ID.
	 */

	if (! (dir = opendir (src_root)))
		return -1;

	if (src_orig == 0) {
		src_orig = src_root;
		dst_orig = dst_root;
		set_orig++;
	}
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

		if (strlen (src_root) + strlen (ent->d_name) + 2 > BUFSIZ) {
			err++;
			break;
		}
		sprintf (src_name, "%s/%s", src_root, ent->d_name);

		if (strlen (dst_root) + strlen (ent->d_name) + 2 > BUFSIZ) {
			err++;
			break;
		}
		sprintf (dst_name, "%s/%s", dst_root, ent->d_name);

		if (stat (src_name, &sb) == -1)
			continue;

		if (S_ISDIR (sb.st_mode)) {

			/*
			 * Create a new target directory, make it owned by
			 * the user and then recursively copy that directory.
			 */

			mkdir (dst_name, sb.st_mode & 0777);
			chown (dst_name, uid == -1 ? sb.st_uid:uid,
				gid == -1 ? sb.st_gid:gid);

			if (copy_tree (src_name, dst_name, uid, gid)) {
				err++;
				break;
			}
			continue;
		}

		/*
		 * See if this is a previously copied link
		 */

		if (lp = check_link (src_name, &sb)) {
			if (link (lp->ln_name, dst_name)) {
				err++;
				break;
			}
			if (unlink (src_name)) {
				err++;
				break;
			}
			if (--lp->ln_count <= 0)
				remove_link (lp);

			continue;
		}

		/*
		 * Deal with FIFOs and special files.  The user really
		 * shouldn't have any of these, but it seems like it
		 * would be nice to copy everything ...
		 */

		if (! S_ISREG (sb.st_mode)) {
			if (mknod (dst_name, sb.st_mode & ~07777, sb.st_rdev) ||
				chown (dst_name, uid == -1 ? sb.st_uid:uid,
					gid == -1 ? sb.st_gid:gid) ||
					chmod (dst_name, sb.st_mode & 07777)) {
				err++;
				break;
			}
			continue;
		}

		/*
		 * Create the new file and copy the contents.  The new
		 * file will be owned by the provided UID and GID values.
		 */

		if ((ifd = open (src_name, O_RDONLY)) < 0) {
			err++;
			break;
		}
		if ((ofd = open (dst_name, O_WRONLY|O_CREAT, 0)) < 0 ||
			chown (dst_name, uid == -1 ? sb.st_uid:uid,
					gid == -1 ? sb.st_gid:gid) ||
				chmod (dst_name, sb.st_mode & 07777)) {
			close (ifd);
			err++;
			break;
		}
		while ((cnt = read (ifd, buf, sizeof buf)) > 0) {
			if (write (ofd, buf, cnt) != cnt) {
				cnt = -1;
				break;
			}
		}
		close (ifd);
		close (ofd);

		if (cnt == -1) {
			err++;
			break;
		}
	}
	closedir (dir);

	if (set_orig) {
		src_orig = 0;
		dst_orig = 0;
	}
	return err ? -1:0;
}

/*
 * remove_tree - remove files in a directory tree
 *
 *	remove_tree() walks a directory tree and deletes all the files
 *	and directories.
 */

int
remove_tree (root)
char	*root;
{
	char	new_name[BUFSIZ];
	int	err = 0;
	struct	DIRECT	*ent;
	struct	stat	sb;
	DIR	*dir;

	/*
	 * Make certain the directory exists.
	 */

	if (access (root, 0) != 0)
		return -1;

	/*
	 * Open the source directory and read each entry.  Every file
	 * entry in the directory is copied with the UID and GID set
	 * to the provided values.  As an added security feature only
	 * regular files (and directories ...) are copied, and no file
	 * is made set-ID.
	 */

	dir = opendir (root);

	while (ent = readdir (dir)) {

		/*
		 * Skip the "." and ".." entries
		 */

		if (strcmp (ent->d_name, ".") == 0 ||
				strcmp (ent->d_name, "..") == 0)
			continue;

		/*
		 * Make the filename for the current entry.
		 */

		if (strlen (root) + strlen (ent->d_name) + 2 > BUFSIZ) {
			err++;
			break;
		}
		sprintf (new_name, "%s/%s", root, ent->d_name);
		if (stat (new_name, &sb) == -1)
			continue;

		if (S_ISDIR (sb.st_mode)) {

			/*
			 * Recursively delete this directory.
			 */

			if (remove_tree (new_name)) {
				err++;
				break;
			}
			if (rmdir (new_name)) {
				err++;
				break;
			}
			continue;
		}
		unlink (new_name);
	}
	closedir (dir);

	return err ? -1:0;
}

#endif	/* defined(DIR_XXX) */
