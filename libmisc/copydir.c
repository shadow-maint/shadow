/*
 * Copyright 1991 - 1994, Julianne Frances Haugh
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

#ident "$Id$"

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdio.h>
#include "prototypes.h"
#include "defines.h"
#ifdef WITH_SELINUX
#include <selinux/selinux.h>
static int selinux_enabled = -1;
#endif
static const char *src_orig;
static const char *dst_orig;

struct link_name {
	dev_t ln_dev;
	ino_t ln_ino;
	int ln_count;
	char *ln_name;
	struct link_name *ln_next;
};
static struct link_name *links;

static int copy_entry (const char *src, const char *dst,
                       uid_t uid, gid_t gid);
static int copy_dir (const char *src, const char *dst,
                     const struct stat *statp, const struct timeval mt[2],
                     uid_t uid, gid_t gid);
#ifdef	S_IFLNK
static int copy_symlink (const char *src, const char *dst,
                         const struct stat *statp, const struct timeval mt[2],
                         uid_t uid, gid_t gid);
#endif
static int copy_hardlink (const char *src, const char *dst,
                          struct link_name *lp);
static int copy_special (const char *src, const char *dst,
                         const struct stat *statp, const struct timeval mt[2],
                         uid_t uid, gid_t gid);
static int copy_file (const char *src, const char *dst,
                      const struct stat *statp, const struct timeval mt[2],
                      uid_t uid, gid_t gid);

#ifdef WITH_SELINUX
/*
 * selinux_file_context - Set the security context before any file or
 *                        directory creation.
 *
 *	selinux_file_context () should be called before any creation of file,
 *	symlink, directory, ...
 *
 */
static int selinux_file_context (const char *dst_name)
{
	security_context_t scontext = NULL;

	if (selinux_enabled < 0) {
		selinux_enabled = is_selinux_enabled () > 0;
	}
	if (selinux_enabled) {
		/* Get the default security context for this file */
		if (matchpathcon (dst_name, 0, &scontext) < 0) {
			if (security_getenforce () != 0) {
				return 1;
			}
		}
		/* Set the security context for the next created file */
		if (setfscreatecon (scontext) < 0) {
			if (security_getenforce () != 0) {
				return 1;
			}
		}
		freecon (scontext);
	}
	return 0;
}
#endif

/*
 * remove_link - delete a link from the linked list
 */
static void remove_link (struct link_name *ln)
{
	struct link_name *lp;

	if (links == ln) {
		links = ln->ln_next;
		free (ln->ln_name);
		free (ln);
		return;
	}
	for (lp = links; NULL !=lp; lp = lp->ln_next) {
		if (lp->ln_next == ln) {
			break;
		}
	}

	if (NULL == lp) {
		return;
	}

	lp->ln_next = lp->ln_next->ln_next;
	free (ln->ln_name);
	free (ln);
}

/*
 * check_link - see if a file is really a link
 */

static struct link_name *check_link (const char *name, const struct stat *sb)
{
	struct link_name *lp;
	int src_len;
	int dst_len;
	int name_len;
	int len;

	for (lp = links; lp; lp = lp->ln_next) {
		if ((lp->ln_dev == sb->st_dev) && (lp->ln_ino == sb->st_ino)) {
			return lp;
		}
	}

	if (sb->st_nlink == 1) {
		return NULL;
	}

	lp = (struct link_name *) xmalloc (sizeof *lp);
	src_len = strlen (src_orig);
	dst_len = strlen (dst_orig);
	name_len = strlen (name);
	lp->ln_dev = sb->st_dev;
	lp->ln_ino = sb->st_ino;
	lp->ln_count = sb->st_nlink;
	len = name_len - src_len + dst_len + 1;
	lp->ln_name = xmalloc (len);
	snprintf (lp->ln_name, len, "%s%s", dst_orig, name + src_len);
	lp->ln_next = links;
	links = lp;

	return NULL;
}

/*
 * copy_tree - copy files in a directory tree
 *
 *	copy_tree() walks a directory tree and copies ordinary files
 *	as it goes.
 */
int copy_tree (const char *src_root, const char *dst_root, uid_t uid, gid_t gid)
{
	char src_name[1024];
	char dst_name[1024];
	int err = 0;
	int set_orig = 0;
	struct DIRECT *ent;
	DIR *dir;

	/*
	 * Make certain both directories exist.  This routine is called
	 * after the home directory is created, or recursively after the
	 * target is created.  It assumes the target directory exists.
	 */

	if (   (access (src_root, F_OK) != 0)
	    || (access (dst_root, F_OK) != 0)) {
		return -1;
	}

	/*
	 * Open the source directory and read each entry.  Every file
	 * entry in the directory is copied with the UID and GID set
	 * to the provided values.  As an added security feature only
	 * regular files (and directories ...) are copied, and no file
	 * is made set-ID.
	 */
	dir = opendir (src_root);
	if (NULL == dir) {
		return -1;
	}

	if (src_orig == 0) {
		src_orig = src_root;
		dst_orig = dst_root;
		set_orig++;
	}
	while ((0 == err) && (ent = readdir (dir)) != NULL) {
		/*
		 * Skip the "." and ".." entries
		 */
		if ((strcmp (ent->d_name, ".") != 0) &&
		    (strcmp (ent->d_name, "..") != 0)) {
			/*
			 * Make sure the resulting source and destination
			 * filenames will fit in their buffers.
			 */
			if (   (strlen (src_root) + strlen (ent->d_name) + 2 >
			        sizeof src_name)
			    || (strlen (dst_root) + strlen (ent->d_name) + 2 >
			        sizeof dst_name)) {
				err = -1;
			} else {
				/*
				 * Build the filename for both the source and
				 * the destination files.
				 */
				snprintf (src_name, sizeof src_name, "%s/%s",
				          src_root, ent->d_name);
				snprintf (dst_name, sizeof dst_name, "%s/%s",
				          dst_root, ent->d_name);

				err = copy_entry (src_name, dst_name, uid, gid);
			}
		}
	}
	closedir (dir);

	if (set_orig) {
		src_orig = 0;
		dst_orig = 0;
	}
	return err;
}

static int copy_entry (const char *src, const char *dst,
                       uid_t uid, gid_t gid)
{
	int err = 0;
	struct stat sb;
	struct link_name *lp;
	struct timeval mt[2];

	if (LSTAT (src, &sb) == -1) {
		/* If we cannot stat the file, do not care. */
	} else {
#if  defined(_BSD_SOURCE) || defined(_SVID_SOURCE)
		mt[0].tv_sec  = sb.st_atim.tv_sec;
		mt[0].tv_usec = sb.st_atim.tv_nsec / 1000;
		mt[1].tv_sec  = sb.st_mtim.tv_sec;
		mt[1].tv_usec = sb.st_mtim.tv_nsec / 1000;
#else
		mt[0].tv_sec  = sb.st_atime;
		mt[0].tv_usec = sb.st_atimensec / 1000;
		mt[1].tv_sec  = sb.st_mtime;
		mt[1].tv_usec = sb.st_mtimensec / 1000;
#endif

		if (S_ISDIR (sb.st_mode)) {
			err = copy_dir (src, dst, &sb, mt, uid, gid);
		}

#ifdef	S_IFLNK
		/*
		 * Copy any symbolic links
		 */

		else if (S_ISLNK (sb.st_mode)) {
			err = copy_symlink (src, dst, &sb, mt, uid, gid);
		}
#endif

		/*
		 * See if this is a previously copied link
		 */

		else if ((lp = check_link (src, &sb)) != NULL) {
			err = copy_hardlink (src, dst, lp);
		}

		/*
		 * Deal with FIFOs and special files.  The user really
		 * shouldn't have any of these, but it seems like it
		 * would be nice to copy everything ...
		 */

		else if (!S_ISREG (sb.st_mode)) {
			err = copy_special (src, dst, &sb, mt, uid, gid);
		}

		/*
		 * Create the new file and copy the contents.  The new
		 * file will be owned by the provided UID and GID values.
		 */

		else {
			err = copy_file (src, dst, &sb, mt, uid, gid);
		}
	}

	return err;
}

static int copy_dir (const char *src, const char *dst,
                     const struct stat *statp, const struct timeval mt[2],
                     uid_t uid, gid_t gid)
{
	int err = 0;

	/*
	 * Create a new target directory, make it owned by
	 * the user and then recursively copy that directory.
	 */

#ifdef WITH_SELINUX
	selinux_file_context (dst);
#endif
	if (   (mkdir (dst, statp->st_mode) != 0)
	    || (chown (dst,
	               (uid == (uid_t) - 1) ? statp->st_uid : uid,
	               (gid == (gid_t) - 1) ? statp->st_gid : gid) != 0)
	    || (chmod (dst, statp->st_mode) != 0)
	    || (copy_tree (src, dst, uid, gid) != 0)
	    || (utimes (dst, mt) != 0)) {
		err = -1;
	}

	return err;
}

#ifdef	S_IFLNK
static int copy_symlink (const char *src, const char *dst,
                         const struct stat *statp, const struct timeval mt[2],
                         uid_t uid, gid_t gid)
{
	char oldlink[1024];
	char dummy[1024];
	int len;
	int err = 0;

	/*
	 * Get the name of the file which the link points
	 * to.  If that name begins with the original
	 * source directory name, that part of the link
	 * name will be replaced with the original
	 * destination directory name.
	 */

	len = readlink (src, oldlink, sizeof (oldlink) - 1);
	if (len < 0) {
		return -1;
	}
	oldlink[len] = '\0';	/* readlink() does not NUL-terminate */
	if (!strncmp (oldlink, src_orig, strlen (src_orig))) {
		snprintf (dummy, sizeof dummy, "%s%s",
		          dst_orig,
		          oldlink + strlen (src_orig));
		strcpy (oldlink, dummy);
	}
#ifdef WITH_SELINUX
	selinux_file_context (dst);
#endif
	if (   (symlink (oldlink, dst) != 0)
	    || (lchown (dst,
	                (uid == (uid_t) - 1) ? statp->st_uid : uid,
	                (gid == (gid_t) - 1) ? statp->st_gid : gid) != 0)) {
		return -1;
	}

	/* 2007-10-18: We don't care about
	 *  exit status of lutimes because
	 *  it returns ENOSYS on many system
	 *  - not implemented
	 */
	lutimes (dst, mt);

	return err;
}
#endif

static int copy_hardlink (const char *src, const char *dst,
                          struct link_name *lp)
{
	/* TODO: selinux needed? */

	if (link (lp->ln_name, dst) != 0) {
		return -1;
	}
	if (unlink (src) != 0) {
		return -1;
	}

	/* If the file could be unlinked, decrement the links counter,
	 * and delete the file if it was the last reference */
	lp->ln_count--;
	if (lp->ln_count <= 0) {
		remove_link (lp);
	}

	return 0;
}

static int copy_special (const char *src, const char *dst,
                         const struct stat *statp, const struct timeval mt[2],
                         uid_t uid, gid_t gid)
{
	int err = 0;

#ifdef WITH_SELINUX
	selinux_file_context (dst);
#endif

	if (   (mknod (dst, statp->st_mode & ~07777, statp->st_rdev) != 0)
	    || (chown (dst,
	               (uid == (uid_t) - 1) ? statp->st_uid : uid,
	               (gid == (gid_t) - 1) ? statp->st_gid : gid) != 0)
	    || (chmod (dst, statp->st_mode & 07777) != 0)
	    || (utimes (dst, mt) != 0)) {
		err = -1;
	}

	return err;
}

static int copy_file (const char *src, const char *dst,
                      const struct stat *statp, const struct timeval mt[2],
                      uid_t uid, gid_t gid)
{
	int err = 0;
	int ifd;
	int ofd;
	char buf[1024];
	int cnt;

	ifd = open (src, O_RDONLY);
	if (ifd < 0) {
		return -1;
	}
#ifdef WITH_SELINUX
	selinux_file_context (dst);
#endif
	ofd = open (dst, O_WRONLY | O_CREAT | O_TRUNC, 0);
	if (   (ofd < 0)
	    || (chown (dst,
	               (uid == (uid_t) - 1) ? statp->st_uid : uid,
	               (gid == (gid_t) - 1) ? statp->st_gid : gid) != 0)
	    || (chmod (dst, statp->st_mode & 07777) != 0)) {
		close (ifd);
		return -1;
	}

	while ((cnt = read (ifd, buf, sizeof buf)) > 0) {
		if (write (ofd, buf, cnt) != cnt) {
			return -1;
		}
	}

	close (ifd);

	if (futimes (ofd, mt) != 0) {
		return -1;
	}

	if (close (ofd) != 0) {
		return -1;
	}

	return err;
}

/*
 * remove_tree - remove files in a directory tree
 *
 *	remove_tree() walks a directory tree and deletes all the files
 *	and directories.
 */

int remove_tree (const char *root)
{
	char new_name[1024];
	int err = 0;
	struct DIRECT *ent;
	struct stat sb;
	DIR *dir;

	/*
	 * Make certain the directory exists.
	 */

	if (access (root, F_OK) != 0) {
		return -1;
	}

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

		if (strlen (root) + strlen (ent->d_name) + 2 > sizeof new_name) {
			err = -1;
			break;
		}
		snprintf (new_name, sizeof new_name, "%s/%s", root,
			  ent->d_name);
		if (LSTAT (new_name, &sb) == -1) {
			continue;
		}

		if (S_ISDIR (sb.st_mode)) {

			/*
			 * Recursively delete this directory.
			 */

			if (remove_tree (new_name)) {
				err = -1;
				break;
			}
			if (rmdir (new_name)) {
				err = -1;
				break;
			}
			continue;
		}
		unlink (new_name);
	}
	closedir (dir);

	return err;
}

