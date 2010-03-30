/*
 * Copyright (c) 1991 - 1994, Julianne Frances Haugh
 * Copyright (c) 1996 - 2001, Marek Michałkiewicz
 * Copyright (c) 2003 - 2006, Tomasz Kłoczko
 * Copyright (c) 2007 - 2010, Nicolas François
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

#include <assert.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdio.h>
#include "prototypes.h"
#include "defines.h"
#ifdef WITH_SELINUX
#include <selinux/selinux.h>
#endif				/* WITH_SELINUX */
#if defined(WITH_ACL) || defined(WITH_ATTR)
#include <attr/error_context.h>
#endif				/* WITH_ACL || WITH_ATTR */
#ifdef WITH_ACL
#include <acl/libacl.h>
#endif				/* WITH_ACL */
#ifdef WITH_ATTR
#include <attr/libattr.h>
#endif				/* WITH_ATTR */

static /*@null@*/const char *src_orig;
static /*@null@*/const char *dst_orig;

struct link_name {
	dev_t ln_dev;
	ino_t ln_ino;
	nlink_t ln_count;
	char *ln_name;
	/*@dependent@*/struct link_name *ln_next;
};
static /*@exposed@*/struct link_name *links;

static int copy_entry (const char *src, const char *dst,
                       long int uid, long int gid);
static int copy_dir (const char *src, const char *dst,
                     const struct stat *statp, const struct timeval mt[],
                     long int uid, long int gid);
#ifdef	S_IFLNK
static char *readlink_malloc (const char *filename);
static int copy_symlink (const char *src, const char *dst,
                         const struct stat *statp, const struct timeval mt[],
                         long int uid, long int gid);
#endif				/* S_IFLNK */
static int copy_hardlink (const char *src, const char *dst,
                          struct link_name *lp);
static int copy_special (const char *src, const char *dst,
                         const struct stat *statp, const struct timeval mt[],
                         long int uid, long int gid);
static int copy_file (const char *src, const char *dst,
                      const struct stat *statp, const struct timeval mt[],
                      long int uid, long int gid);

#ifdef WITH_SELINUX
/*
 * selinux_file_context - Set the security context before any file or
 *                        directory creation.
 *
 *	selinux_file_context () should be called before any creation of file,
 *	symlink, directory, ...
 *
 *	Callers may have to Reset SELinux to create files with default
 *	contexts:
 *		setfscreatecon (NULL);
 */
int selinux_file_context (const char *dst_name)
{
	static bool selinux_checked = false;
	static bool selinux_enabled;
	security_context_t scontext = NULL;

	if (!selinux_checked) {
		selinux_enabled = is_selinux_enabled () > 0;
		selinux_checked = true;
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
#endif				/* WITH_SELINUX */

#if defined(WITH_ACL) || defined(WITH_ATTR)
void error (struct error_context *ctx, const char *fmt, ...)
{
	va_list ap;

	va_start (ap, fmt);
	(void) fprintf (stderr, _("%s: "), Prog);
	if (vfprintf (stderr, fmt, ap) != 0) {
		(void) fputs (_(": "), stderr);
	}
	(void) fprintf (stderr, "%s\n", strerror (errno));
	va_end (ap);
}

struct error_context ctx = {
	error
};
#endif				/* WITH_ACL || WITH_ATTR */

/*
 * remove_link - delete a link from the linked list
 */
static void remove_link (/*@only@*/struct link_name *ln)
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
		free (ln->ln_name);
		free (ln);
		return;
	}

	lp->ln_next = lp->ln_next->ln_next;
	free (ln->ln_name);
	free (ln);
}

/*
 * check_link - see if a file is really a link
 */

static /*@exposed@*/ /*@null@*/struct link_name *check_link (const char *name, const struct stat *sb)
{
	struct link_name *lp;
	size_t src_len;
	size_t dst_len;
	size_t name_len;
	size_t len;

	/* copy_tree () must be the entry point */
	assert (NULL != src_orig);
	assert (NULL != dst_orig);

	for (lp = links; NULL != lp; lp = lp->ln_next) {
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
	lp->ln_name = (char *) xmalloc (len);
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
int copy_tree (const char *src_root, const char *dst_root,
               long int uid, long int gid)
{
	int err = 0;
	bool set_orig = false;
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

	if (src_orig == NULL) {
		src_orig = src_root;
		dst_orig = dst_root;
		set_orig = true;
	}
	while ((0 == err) && (ent = readdir (dir)) != NULL) {
		/*
		 * Skip the "." and ".." entries
		 */
		if ((strcmp (ent->d_name, ".") != 0) &&
		    (strcmp (ent->d_name, "..") != 0)) {
			char *src_name;
			char *dst_name;
			size_t src_len = strlen (ent->d_name) + 2;
			size_t dst_len = strlen (ent->d_name) + 2;
			src_len += strlen (src_root);
			dst_len += strlen (dst_root);

			src_name = (char *) malloc (src_len);
			dst_name = (char *) malloc (dst_len);

			if ((NULL == src_name) || (NULL == dst_name)) {
				err = -1;
			} else {
				/*
				 * Build the filename for both the source and
				 * the destination files.
				 */
				snprintf (src_name, src_len, "%s/%s",
				          src_root, ent->d_name);
				snprintf (dst_name, dst_len, "%s/%s",
				          dst_root, ent->d_name);

				err = copy_entry (src_name, dst_name, uid, gid);
			}
			if (NULL != src_name) {
				free (src_name);
			}
			if (NULL != dst_name) {
				free (dst_name);
			}
		}
	}
	(void) closedir (dir);

	if (set_orig) {
		src_orig = NULL;
		dst_orig = NULL;
		/* FIXME: clean links
		 * Since there can be hardlinks elsewhere on the device,
		 * we cannot check that all the hardlinks were found:
		assert (NULL == links);
		 */
	}

#ifdef WITH_SELINUX
	/* Reset SELinux to create files with default contexts */
	setfscreatecon (NULL);
#endif				/* WITH_SELINUX */

	return err;
}

/*
 * copy_entry - copy the entry of a directory
 *
 *	Copy the entry src to dst.
 *	Depending on the type of entry, this function will forward the
 *	request to copy_dir(), copy_symlink(), copy_hardlink(),
 *	copy_special(), or copy_file().
 *
 *	The access and modification time will not be modified.
 *
 *	The permissions will be set to uid/gid.
 *
 *	If uid (resp. gid) is equal to -1, the user (resp. group) will
 *	not be modified.
 */
static int copy_entry (const char *src, const char *dst,
                       long int uid, long int gid)
{
	int err = 0;
	struct stat sb;
	struct link_name *lp;
	struct timeval mt[2];

	if (LSTAT (src, &sb) == -1) {
		/* If we cannot stat the file, do not care. */
	} else {
#ifdef HAVE_STRUCT_STAT_ST_ATIM
		mt[0].tv_sec  = sb.st_atim.tv_sec;
		mt[0].tv_usec = sb.st_atim.tv_nsec / 1000;
#else				/* !HAVE_STRUCT_STAT_ST_ATIM */
		mt[0].tv_sec  = sb.st_atime;
# ifdef HAVE_STRUCT_STAT_ST_ATIMENSEC
		mt[0].tv_usec = sb.st_atimensec / 1000;
# else				/* !HAVE_STRUCT_STAT_ST_ATIMENSEC */
		mt[0].tv_usec = 0;
# endif				/* !HAVE_STRUCT_STAT_ST_ATIMENSEC */
#endif				/* !HAVE_STRUCT_STAT_ST_ATIM */

#ifdef HAVE_STRUCT_STAT_ST_MTIM
		mt[1].tv_sec  = sb.st_mtim.tv_sec;
		mt[1].tv_usec = sb.st_mtim.tv_nsec / 1000;
#else				/* !HAVE_STRUCT_STAT_ST_MTIM */
		mt[1].tv_sec  = sb.st_mtime;
# ifdef HAVE_STRUCT_STAT_ST_MTIMENSEC
		mt[1].tv_usec = sb.st_mtimensec / 1000;
# else				/* !HAVE_STRUCT_STAT_ST_MTIMENSEC */
		mt[1].tv_usec = 0;
# endif				/* !HAVE_STRUCT_STAT_ST_MTIMENSEC */
#endif				/* !HAVE_STRUCT_STAT_ST_MTIM */

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
#endif				/* S_IFLNK */

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

/*
 * copy_dir - copy a directory
 *
 *	Copy a directory (recursively) from src to dst.
 *
 *	statp, mt, uid, gid are used to set the access and modification and the
 *	access rights.
 *
 *	Return 0 on success, -1 on error.
 */
static int copy_dir (const char *src, const char *dst,
                     const struct stat *statp, const struct timeval mt[],
                     long int uid, long int gid)
{
	int err = 0;

	/*
	 * Create a new target directory, make it owned by
	 * the user and then recursively copy that directory.
	 */

#ifdef WITH_SELINUX
	selinux_file_context (dst);
#endif				/* WITH_SELINUX */
	if (   (mkdir (dst, statp->st_mode) != 0)
	    || (chown (dst,
	               (uid == - 1) ? statp->st_uid : (uid_t) uid,
	               (gid == - 1) ? statp->st_gid : (gid_t) gid) != 0)
#ifdef WITH_ACL
	    || (perm_copy_file (src, dst, &ctx) != 0)
#else				/* !WITH_ACL */
	    || (chmod (dst, statp->st_mode) != 0)
#endif				/* !WITH_ACL */
#ifdef WITH_ATTR
	/*
	 * If the third parameter is NULL, all extended attributes
	 * except those that define Access Control Lists are copied.
	 * ACLs are excluded by default because copying them between
	 * file systems with and without ACL support needs some
	 * additional logic so that no unexpected permissions result.
	 */
	    || (attr_copy_file (src, dst, NULL, &ctx) != 0)
#endif				/* WITH_ATTR */
	    || (copy_tree (src, dst, uid, gid) != 0)
	    || (utimes (dst, mt) != 0)) {
		err = -1;
	}

	return err;
}

#ifdef	S_IFLNK
/*
 * readlink_malloc - wrapper for readlink
 *
 * return NULL on error.
 * The return string shall be freed by the caller.
 */
static char *readlink_malloc (const char *filename)
{
	size_t size = 1024;

	while (1) {
		ssize_t nchars;
		char *buffer = (char *) malloc (size);
		if (NULL == buffer) {
			return NULL;
		}

		nchars = readlink (filename, buffer, size);

		if (nchars < 0) {
			free(buffer);
			return NULL;
		}

		if ( (size_t) nchars < size) { /* The buffer was large enough */
			/* readlink does not nul-terminate */
			buffer[nchars] = '\0';
			return buffer;
		}

		/* Try again with a bigger buffer */
		free (buffer);
		size *= 2;
	}
}

/*
 * copy_symlink - copy a symlink
 *
 *	Copy a symlink from src to dst.
 *
 *	statp, mt, uid, gid are used to set the access and modification and the
 *	access rights.
 *
 *	Return 0 on success, -1 on error.
 */
static int copy_symlink (const char *src, const char *dst,
                         const struct stat *statp, const struct timeval mt[],
                         long int uid, long int gid)
{
	char *oldlink;

	/* copy_tree () must be the entry point */
	assert (NULL != src_orig);
	assert (NULL != dst_orig);

	/*
	 * Get the name of the file which the link points
	 * to.  If that name begins with the original
	 * source directory name, that part of the link
	 * name will be replaced with the original
	 * destination directory name.
	 */

	oldlink = readlink_malloc (src);
	if (NULL == oldlink) {
		return -1;
	}

	/* If src was a link to an entry of the src_orig directory itself,
	 * create a link to the corresponding entry in the dst_orig
	 * directory.
	 */
	if (strncmp (oldlink, src_orig, strlen (src_orig)) == 0) {
		size_t len = strlen (dst_orig) + strlen (oldlink) - strlen (src_orig) + 1;
		char *dummy = (char *) malloc (len);
		snprintf (dummy, len, "%s%s",
		          dst_orig,
		          oldlink + strlen (src_orig));
		free (oldlink);
		oldlink = dummy;
	}

#ifdef WITH_SELINUX
	selinux_file_context (dst);
#endif				/* WITH_SELINUX */
	if (   (symlink (oldlink, dst) != 0)
	    || (lchown (dst,
	                (uid == -1) ? statp->st_uid : (uid_t) uid,
	                (gid == -1) ? statp->st_gid : (gid_t) gid) != 0)) {
		/* FIXME: there are no modes on symlinks, right?
		 *        ACL could be copied, but this would be much more
		 *        complex than calling perm_copy_file.
		 *        Ditto for Extended Attributes.
		 *        We currently only document that ACL and Extended
		 *        Attributes are not copied.
		 */
		free (oldlink);
		return -1;
	}
	free (oldlink);

#ifdef HAVE_LUTIMES
	/* 2007-10-18: We don't care about
	 *  exit status of lutimes because
	 *  it returns ENOSYS on many system
	 *  - not implemented
	 */
	lutimes (dst, mt);
#endif				/* HAVE_LUTIMES */

	return 0;
}
#endif				/* S_IFLNK */

/*
 * copy_hardlink - copy a hardlink
 *
 *	Copy a hardlink from src to dst.
 *
 *	Return 0 on success, -1 on error.
 */
static int copy_hardlink (const char *src, const char *dst,
                          struct link_name *lp)
{
	/* FIXME: selinux, ACL, Extended Attributes needed? */

	if (link (lp->ln_name, dst) != 0) {
		return -1;
	}

	/* If the file could be unlinked, decrement the links counter,
	 * and forget about this link if it was the last reference */
	lp->ln_count--;
	if (lp->ln_count <= 0) {
		remove_link (lp);
	}

	return 0;
}

/*
 * copy_special - copy a special file
 *
 *	Copy a special file from src to dst.
 *
 *	statp, mt, uid, gid are used to set the access and modification and the
 *	access rights.
 *
 *	Return 0 on success, -1 on error.
 */
static int copy_special (const char *src, const char *dst,
                         const struct stat *statp, const struct timeval mt[],
                         long int uid, long int gid)
{
	int err = 0;

#ifdef WITH_SELINUX
	selinux_file_context (dst);
#endif				/* WITH_SELINUX */

	if (   (mknod (dst, statp->st_mode & ~07777, statp->st_rdev) != 0)
	    || (chown (dst,
	               (uid == -1) ? statp->st_uid : (uid_t) uid,
	               (gid == -1) ? statp->st_gid : (gid_t) gid) != 0)
#ifdef WITH_ACL
	    || (perm_copy_file (src, dst, &ctx) != 0)
#else				/* !WITH_ACL */
	    || (chmod (dst, statp->st_mode & 07777) != 0)
#endif				/* !WITH_ACL */
#ifdef WITH_ATTR
	/*
	 * If the third parameter is NULL, all extended attributes
	 * except those that define Access Control Lists are copied.
	 * ACLs are excluded by default because copying them between
	 * file systems with and without ACL support needs some
	 * additional logic so that no unexpected permissions result.
	 */
	    || (attr_copy_file (src, dst, NULL, &ctx) != 0)
#endif				/* WITH_ATTR */
	    || (utimes (dst, mt) != 0)) {
		err = -1;
	}

	return err;
}

/*
 * copy_file - copy a file
 *
 *	Copy a file from src to dst.
 *
 *	statp, mt, uid, gid are used to set the access and modification and the
 *	access rights.
 *
 *	Return 0 on success, -1 on error.
 */
static int copy_file (const char *src, const char *dst,
                      const struct stat *statp, const struct timeval mt[],
                      long int uid, long int gid)
{
	int err = 0;
	int ifd;
	int ofd;
	char buf[1024];
	ssize_t cnt;

	ifd = open (src, O_RDONLY);
	if (ifd < 0) {
		return -1;
	}
#ifdef WITH_SELINUX
	selinux_file_context (dst);
#endif				/* WITH_SELINUX */
	ofd = open (dst, O_WRONLY | O_CREAT | O_TRUNC, statp->st_mode & 07777);
	if (   (ofd < 0)
	    || (fchown (ofd,
	                (uid == -1) ? statp->st_uid : (uid_t) uid,
	                (gid == -1) ? statp->st_gid : (gid_t) gid) != 0)
#ifdef WITH_ACL
	    || (perm_copy_fd (src, ifd, dst, ofd, &ctx) != 0)
#else				/* !WITH_ACL */
	    || (fchmod (ofd, statp->st_mode & 07777) != 0)
#endif				/* !WITH_ACL */
#ifdef WITH_ATTR
	/*
	 * If the third parameter is NULL, all extended attributes
	 * except those that define Access Control Lists are copied.
	 * ACLs are excluded by default because copying them between
	 * file systems with and without ACL support needs some
	 * additional logic so that no unexpected permissions result.
	 */
	    || (attr_copy_fd (src, ifd, dst, ofd, NULL, &ctx) != 0)
#endif				/* WITH_ATTR */
	   ) {
		(void) close (ifd);
		return -1;
	}

	while ((cnt = read (ifd, buf, sizeof buf)) > 0) {
		if (write (ofd, buf, (size_t)cnt) != cnt) {
			(void) close (ifd);
			return -1;
		}
	}

	(void) close (ifd);

#ifdef HAVE_FUTIMES
	if (futimes (ofd, mt) != 0) {
		return -1;
	}
#endif				/* HAVE_FUTIMES */

	if (close (ofd) != 0) {
		return -1;
	}

#ifndef HAVE_FUTIMES
	if (utimes(dst, mt) != 0) {
		return -1;
	}
#endif				/* !HAVE_FUTIMES */

	return err;
}

