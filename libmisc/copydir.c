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

#ident "$Id: copydir.c,v 1.14 2006/05/07 18:10:10 kloczek Exp $"

#include <sys/stat.h>
#include <sys/types.h>
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

#ifdef WITH_SELINUX
static int selinux_file_context (const char *dst_name)
{
	security_context_t scontext = NULL;

	if (selinux_enabled < 0)
		selinux_enabled = is_selinux_enabled () > 0;
	if (selinux_enabled) {
		if (matchpathcon (dst_name, 0, &scontext) < 0)
			if (security_getenforce ())
				return 1;
		if (setfscreatecon (scontext) < 0)
			if (security_getenforce ())
				return 1;
		freecon (scontext);
	}
	return 0;
}
#endif

/*
 * remove_link - delete a link from the link list
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
	for (lp = links; lp; lp = lp->ln_next)
		if (lp->ln_next == ln)
			break;

	if (!lp)
		return;

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

	for (lp = links; lp; lp = lp->ln_next)
		if (lp->ln_dev == sb->st_dev && lp->ln_ino == sb->st_ino)
			return lp;

	if (sb->st_nlink == 1)
		return 0;

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

	return 0;
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
	char buf[1024];
	int ifd;
	int ofd;
	int err = 0;
	int cnt;
	int set_orig = 0;
	struct DIRECT *ent;
	struct stat sb;
	struct link_name *lp;
	DIR *dir;

	/*
	 * Make certain both directories exist.  This routine is called
	 * after the home directory is created, or recursively after the
	 * target is created.  It assumes the target directory exists.
	 */

	if (access (src_root, F_OK) != 0 || access (dst_root, F_OK) != 0)
		return -1;

	/*
	 * Open the source directory and read each entry.  Every file
	 * entry in the directory is copied with the UID and GID set
	 * to the provided values.  As an added security feature only
	 * regular files (and directories ...) are copied, and no file
	 * is made set-ID.
	 */

	if (!(dir = opendir (src_root)))
		return -1;

	if (src_orig == 0) {
		src_orig = src_root;
		dst_orig = dst_root;
		set_orig++;
	}
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

		if (strlen (src_root) + strlen (ent->d_name) + 2 >
		    sizeof src_name) {
			err++;
			break;
		}
		snprintf (src_name, sizeof src_name, "%s/%s", src_root,
			  ent->d_name);

		if (strlen (dst_root) + strlen (ent->d_name) + 2 >
		    sizeof dst_name) {
			err++;
			break;
		}
		snprintf (dst_name, sizeof dst_name, "%s/%s", dst_root,
			  ent->d_name);

		if (LSTAT (src_name, &sb) == -1)
			continue;

		if (S_ISDIR (sb.st_mode)) {

			/*
			 * Create a new target directory, make it owned by
			 * the user and then recursively copy that directory.
			 */

#ifdef WITH_SELINUX
			selinux_file_context (dst_name);
#endif
			if (mkdir (dst_name, sb.st_mode)
			    || chown (dst_name,
				      uid == (uid_t) - 1 ? sb.st_uid : uid,
				      gid == (gid_t) - 1 ? sb.st_gid : gid)
			    || chmod (dst_name, sb.st_mode)
			    || copy_tree (src_name, dst_name, uid, gid)) {
				err++;
				break;
			}
			continue;
		}
#ifdef	S_IFLNK
		/*
		 * Copy any symbolic links
		 */

		if (S_ISLNK (sb.st_mode)) {
			char oldlink[1024];
			char dummy[1024];
			int len;

			/*
			 * Get the name of the file which the link points
			 * to.  If that name begins with the original
			 * source directory name, that part of the link
			 * name will be replaced with the original
			 * destinateion directory name.
			 */

			if ((len =
			     readlink (src_name, oldlink,
				       sizeof (oldlink) - 1)) < 0) {
				err++;
				break;
			}
			oldlink[len] = '\0';	/* readlink() does not NUL-terminate */
			if (!strncmp (oldlink, src_orig, strlen (src_orig))) {
				snprintf (dummy, sizeof dummy, "%s%s",
					  dst_orig,
					  oldlink + strlen (src_orig));
				strcpy (oldlink, dummy);
			}
#ifdef WITH_SELINUX
			selinux_file_context (dst_name);
#endif
			if (symlink (oldlink, dst_name) ||
			    lchown (dst_name,
				    uid == (uid_t) - 1 ? sb.st_uid : uid,
				    gid == (gid_t) - 1 ? sb.st_gid : gid)) {
				err++;
				break;
			}
			continue;
		}
#endif

		/*
		 * See if this is a previously copied link
		 */

		if ((lp = check_link (src_name, &sb))) {
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

		if (!S_ISREG (sb.st_mode)) {
#ifdef WITH_SELINUX
			selinux_file_context (dst_name);
#endif
			if (mknod (dst_name, sb.st_mode & ~07777, sb.st_rdev)
			    || chown (dst_name,
				      uid == (uid_t) - 1 ? sb.st_uid : uid,
				      gid == (gid_t) - 1 ? sb.st_gid : gid)
			    || chmod (dst_name, sb.st_mode & 07777)) {
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
#ifdef WITH_SELINUX
		selinux_file_context (dst_name);
#endif
		if ((ofd =
		     open (dst_name, O_WRONLY | O_CREAT | O_TRUNC, 0)) < 0
		    || chown (dst_name,
			      uid == (uid_t) - 1 ? sb.st_uid : uid,
			      gid == (gid_t) - 1 ? sb.st_gid : gid)
		    || chmod (dst_name, sb.st_mode & 07777)) {
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
	return err ? -1 : 0;
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

	if (access (root, F_OK) != 0)
		return -1;

	/*
	 * Open the source directory and read each entry.  Every file
	 * entry in the directory is copied with the UID and GID set
	 * to the provided values.  As an added security feature only
	 * regular files (and directories ...) are copied, and no file
	 * is made set-ID.
	 */

	dir = opendir (root);

	while ((ent = readdir (dir))) {

		/*
		 * Skip the "." and ".." entries
		 */

		if (strcmp (ent->d_name, ".") == 0 ||
		    strcmp (ent->d_name, "..") == 0)
			continue;

		/*
		 * Make the filename for the current entry.
		 */

		if (strlen (root) + strlen (ent->d_name) + 2 > sizeof new_name) {
			err++;
			break;
		}
		snprintf (new_name, sizeof new_name, "%s/%s", root,
			  ent->d_name);
		if (LSTAT (new_name, &sb) == -1)
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

	return err ? -1 : 0;
}
