/*
 * Copyright 1990, 1991, 1992, 1993, John F. Haugh II
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
 *
 *	This file implements a transaction oriented shadow group
 *	database library.  The shadow group file is updated one
 *	entry at a time.  After each transaction the file must be
 *	logically closed and transferred to the existing shadow
 *	group file.  The sequence of events is
 *
 *	sgr_lock			-- lock shadow group file
 *	sgr_open			-- logically open shadow group file
 *	while transaction to process
 *		sgr_(locate,update,remove) -- perform transaction
 *	done
 *	sgr_close			-- commit transactions
 *	sgr_unlock			-- remove shadow group lock
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#ifdef	BSD
#include <strings.h>
#define	strchr	index
#define	strrchr	rindex
#else
#include <string.h>
#endif
#include "shadow.h"

#ifndef	lint
static	char	sccsid[] = "@(#)sgroupio.c	3.7	07:49:53	06 May 1993";
#endif

static	int	islocked;
static	int	isopen;
static	int	open_modes;
static	FILE	*sgrfp;

struct	sg_file_entry {
	char	*sgr_line;
	int	sgr_changed;
	struct	sgrp	*sgr_entry;
	struct	sg_file_entry *sgr_next;
};

struct	sg_file_entry	*__sgr_head;
static	struct	sg_file_entry	*sgr_tail;
static	struct	sg_file_entry	*sgr_cursor;
int	__sg_changed;
static	int	lock_pid;

#define	SG_LOCK	"/etc/gshadow.lock"
#define	GR_TEMP "/etc/gshadow.%d"
#define	SGROUP	"/etc/gshadow"

static	char	sg_filename[BUFSIZ] = SGROUP;

extern	char	*strdup();
extern	struct	sgrp	*sgetsgent();
extern	char	*fgetsx();
extern	char	*malloc();

/*
 * sgr_dup - duplicate a shadow group file entry
 *
 *	sgr_dup() accepts a pointer to a shadow group file entry and
 *	returns a pointer to a shadow group file entry in allocated memory.
 */

static struct sgrp *
sgr_dup (sgrent)
struct	sgrp	*sgrent;
{
	struct	sgrp	*sgr;
	int	i;

	if (! (sgr = (struct sgrp *) malloc (sizeof *sgr)))
		return 0;

	if ((sgr->sg_name = strdup (sgrent->sg_name)) == 0 ||
			(sgr->sg_passwd = strdup (sgrent->sg_passwd)) == 0)
		return 0;

	for (i = 0;sgrent->sg_mem[i];i++)
		;

	sgr->sg_mem = (char **) malloc (sizeof (char *) * (i + 1));
	for (i = 0;sgrent->sg_mem[i];i++)
		if (! (sgr->sg_mem[i] = strdup (sgrent->sg_mem[i])))
			return 0;

	sgr->sg_mem[i] = 0;

	for (i = 0;sgrent->sg_adm[i];i++)
		;

	sgr->sg_adm = (char **) malloc (sizeof (char *) * (i + 1));
	for (i = 0;sgrent->sg_adm[i];i++)
		if (! (sgr->sg_adm[i] = strdup (sgrent->sg_adm[i])))
			return 0;

	sgr->sg_adm[i] = 0;

	return sgr;
}

/*
 * sgr_free - free a dynamically allocated shadow group file entry
 *
 *	sgr_free() frees up the memory which was allocated for the
 *	pointed to entry.
 */

static void
sgr_free (sgrent)
struct	sgrp	*sgrent;
{
	int	i;

	free (sgrent->sg_name);
	free (sgrent->sg_passwd);

	for (i = 0;sgrent->sg_mem[i];i++)
		free (sgrent->sg_mem[i]);

	free (sgrent->sg_mem);

	for (i = 0;sgrent->sg_adm[i];i++)
		free (sgrent->sg_adm[i]);

	free (sgrent->sg_adm);
}

/*
 * sgr_name - change the name of the shadow group file
 */

int
sgr_name (name)
char	*name;
{
	if (isopen || strlen (name) > (BUFSIZ-10))
		return -1;

	strcpy (sg_filename, name);
	return 0;
}

/*
 * sgr_lock - lock a shadow group file
 *
 *	sgr_lock() encapsulates the lock operation.  it returns
 *	TRUE or FALSE depending on the shadow group file being
 *	properly locked.  the lock is set by creating a semaphore
 *	file, SG_LOCK.
 */

int
sgr_lock ()
{
	int	fd;
	int	pid;
	int	len;
	char	file[BUFSIZ];
	char	buf[32];
	struct	stat	sb;

	if (islocked)
		return 1;

	if (strcmp (sg_filename, SGROUP) != 0)
		return 0;

	/*
	 * Create a lock file which can be switched into place
	 */

	sprintf (file, GR_TEMP, lock_pid = getpid ());
	if ((fd = open (file, O_CREAT|O_EXCL|O_WRONLY, 0600)) == -1)
		return 0;

	sprintf (buf, "%d", lock_pid);
	if (write (fd, buf, strlen (buf) + 1) != strlen (buf) + 1) {
		(void) close (fd);
		(void) unlink (file);
		return 0;
	}
	close (fd);

	/*
	 * Simple case first -
	 *	Link fails (in a sane environment ...) if the target
	 *	exists already.  So we try to switch in a new lock
	 *	file.  If that succeeds, we assume we have the only
	 *	valid lock.  Needs work for NFS where this assumption
	 *	may not hold.  The simple hack is to check the link
	 *	count on the source file, which should be 2 iff the
	 *	link =really= worked.
	 */

	if (link (file, SG_LOCK) == 0) {
		if (stat (file, &sb) != 0)
			return 0;

		if (sb.st_nlink != 2)
			return 0;

		(void) unlink (file);
		islocked = 1;
		return 1;
	}

	/*
	 * Invalid lock test -
	 *	Open the lock file and see if the lock is valid.
	 *	The PID of the lock file is checked, and if the PID
	 *	is not valid, the lock file is removed.  If the unlink
	 *	of the lock file fails, it should mean that someone
	 *	else is executing this code.  They will get success,
	 *	and we will fail.
	 */

	if ((fd = open (SG_LOCK, O_RDWR)) == -1 ||
			(len = read (fd, buf, BUFSIZ)) <= 0) {
		errno = EINVAL;
		return 0;
	}
	buf[len] = '\0';
	if ((pid = strtol (buf, (char **) 0, 10)) == 0) {
		errno = EINVAL;
		return 0;
	}
	if (kill (pid, 0) == 0)  {
		errno = EEXIST;
		return 0;
	}
	if (unlink (SG_LOCK)) {
		(void) close (fd);
		(void) unlink (file);

		return 0;
	}

	/*
	 * Re-try lock -
	 *	The invalid lock has now been removed and I should
	 *	be able to acquire a lock for myself just fine.  If
	 *	this fails there will be no retry.  The link count
	 *	test here makes certain someone executing the previous
	 *	block of code didn't just remove the lock we just
	 *	linked to.
	 */

	if (link (file, SG_LOCK) == 0) {
		if (stat (file, &sb) != 0)
			return 0;

		if (sb.st_nlink != 2)
			return 0;

		(void) unlink (file);
		islocked = 1;
		return 1;
	}
	(void) unlink (file);
	return 0;
}

/*
 * sgr_unlock - logically unlock a shadow group file
 *
 *	sgr_unlock() removes the lock which was set by an earlier
 *	invocation of sgr_lock().
 */

int
sgr_unlock ()
{
	if (isopen) {
		open_modes = O_RDONLY;
		if (! sgr_close ())
			return 0;
	}
	if (islocked) {
		islocked = 0;
		if (lock_pid != getpid ())
			return 0;

		(void) unlink (SG_LOCK);
		return 1;
	}
	return 0;
}

/*
 * sgr_open - open a shadow group file
 *
 *	sgr_open() encapsulates the open operation.  it returns
 *	TRUE or FALSE depending on the shadow group file being
 *	properly opened.
 */

int
sgr_open (mode)
int	mode;
{
	char	buf[8192];
	char	*cp;
	struct	sg_file_entry	*sgrf;
	struct	sgrp	*sgrent;

	if (isopen || (mode != O_RDONLY && mode != O_RDWR))
		return 0;

	if (mode != O_RDONLY && ! islocked &&
			strcmp (sg_filename, SGROUP) == 0)
		return 0;

	if ((sgrfp = fopen (sg_filename, mode == O_RDONLY ? "r":"r+")) == 0)
		return 0;

	__sgr_head = sgr_tail = sgr_cursor = 0;
	__sg_changed = 0;

	while (fgetsx (buf, sizeof buf, sgrfp) != (char *) 0) {
		if (cp = strrchr (buf, '\n'))
			*cp = '\0';

		if (! (sgrf = (struct sg_file_entry *) malloc (sizeof *sgrf)))
			return 0;

		sgrf->sgr_changed = 0;
		sgrf->sgr_line = strdup (buf);
		if ((sgrent = sgetsgent (buf)) && ! (sgrent = sgr_dup (sgrent)))
			return 0;

		sgrf->sgr_entry = sgrent;

		if (__sgr_head == 0) {
			__sgr_head = sgr_tail = sgrf;
			sgrf->sgr_next = 0;
		} else {
			sgr_tail->sgr_next = sgrf;
			sgrf->sgr_next = 0;
			sgr_tail = sgrf;
		}
	}
	isopen++;
	open_modes = mode;

	return 1;
}

/*
 * sgr_close - close the shadow group file
 *
 *	sgr_close() outputs any modified shadow group file entries and
 *	frees any allocated memory.
 */

int
sgr_close ()
{
	char	backup[BUFSIZ];
	int	mask;
	int	c;
	int	errors = 0;
	FILE	*bkfp;
	struct	sg_file_entry *sgrf;
	struct	stat	sb;

	if (! isopen) {
		errno = EINVAL;
		return 0;
	}
	if (islocked && lock_pid != getpid ()) {
		isopen = 0;
		islocked = 0;
		errno = EACCES;
		return 0;
	}
	strcpy (backup, sg_filename);
	strcat (backup, "-");

	if (open_modes == O_RDWR && __sg_changed) {
		mask = umask (0377);
		(void) unlink (backup);
		if ((bkfp = fopen (backup, "w")) == 0) {
			umask (mask);
			return 0;
		}
		umask (mask);
		(void) chmod (backup, 0400);
		fstat (fileno (sgrfp), &sb);
		chown (backup, sb.st_uid, sb.st_gid);

		rewind (sgrfp);
		while ((c = getc (sgrfp)) != EOF) {
			if (putc (c, bkfp) == EOF) {
				fclose (bkfp);
				return 0;
			}
		}
		if (fclose (bkfp))
			return 0;

		isopen = 0;
		(void) fclose (sgrfp);

		mask = umask (0277);
		(void) chmod (sg_filename, 0400);
		if (! (sgrfp = fopen (sg_filename, "w"))) {
			umask (mask);
			return 0;
		}
		umask (mask);

		for (sgrf = __sgr_head;! errors && sgrf;sgrf = sgrf->sgr_next) {
			if (sgrf->sgr_changed) {
				if (putsgent (sgrf->sgr_entry, sgrfp))
					errors++;
			} else {
				if (fputsx (sgrf->sgr_line, sgrfp))
					errors++;

				if (putc ('\n', sgrfp) == EOF)
					errors++;
			}
		}
		if (fflush (sgrfp))
			errors++;

		if (errors) {
			unlink (sg_filename);
			link (backup, sg_filename);
			unlink (backup);
			return 0;
		}
	}
	if (fclose (sgrfp))
		return 0;

	sgrfp = 0;

	while (__sgr_head != 0) {
		sgrf = __sgr_head;
		__sgr_head = sgrf->sgr_next;

		if (sgrf->sgr_entry) {
			sgr_free (sgrf->sgr_entry);
			free (sgrf->sgr_entry);
		}
		if (sgrf->sgr_line)
			free (sgrf->sgr_line);

		free (sgrf);
	}
	sgr_tail = 0;
	isopen = 0;
	return 1;
}

int
sgr_update (sgrent)
struct	sgrp	*sgrent;
{
	struct	sg_file_entry	*sgrf;
	struct	sgrp	*nsgr;

	if (! isopen || open_modes == O_RDONLY) {
		errno = EINVAL;
		return 0;
	}
	for (sgrf = __sgr_head;sgrf != 0;sgrf = sgrf->sgr_next) {
		if (sgrf->sgr_entry == 0)
			continue;

		if (strcmp (sgrent->sg_name, sgrf->sgr_entry->sg_name) != 0)
			continue;

		if (! (nsgr = sgr_dup (sgrent)))
			return 0;
		else {
			sgr_free (sgrf->sgr_entry);
			*(sgrf->sgr_entry) = *nsgr;
		}
		sgrf->sgr_changed = 1;
		sgr_cursor = sgrf;
		return __sg_changed = 1;
	}
	sgrf = (struct sg_file_entry *) malloc (sizeof *sgrf);
	if (! (sgrf->sgr_entry = sgr_dup (sgrent)))
		return 0;

	sgrf->sgr_changed = 1;
	sgrf->sgr_next = 0;
	sgrf->sgr_line = 0;

	if (sgr_tail)
		sgr_tail->sgr_next = sgrf;

	if (! __sgr_head)
		__sgr_head = sgrf;

	sgr_tail = sgrf;

	return __sg_changed = 1;
}

int
sgr_remove (name)
char	*name;
{
	struct	sg_file_entry	*sgrf;
	struct	sg_file_entry	*osgrf;

	if (! isopen || open_modes == O_RDONLY) {
		errno = EINVAL;
		return 0;
	}
	for (osgrf = 0, sgrf = __sgr_head;sgrf != 0;
			osgrf = sgrf, sgrf = sgrf->sgr_next) {
		if (! sgrf->sgr_entry)
			continue;

		if (strcmp (name, sgrf->sgr_entry->sg_name) != 0)
			continue;

		if (sgrf == sgr_cursor)
			sgr_cursor = osgrf;

		if (osgrf != 0)
			osgrf->sgr_next = sgrf->sgr_next;
		else
			__sgr_head = sgrf->sgr_next;

		if (sgrf == sgr_tail)
			sgr_tail = osgrf;

		return __sg_changed = 1;
	}
	errno = ENOENT;
	return 0;
}

struct sgrp *
sgr_locate (name)
char	*name;
{
	struct	sg_file_entry	*sgrf;

	if (! isopen) {
		errno = EINVAL;
		return 0;
	}
	for (sgrf = __sgr_head;sgrf != 0;sgrf = sgrf->sgr_next) {
		if (sgrf->sgr_entry == 0)
			continue;

		if (strcmp (name, sgrf->sgr_entry->sg_name) == 0) {
			sgr_cursor = sgrf;
			return sgrf->sgr_entry;
		}
	}
	errno = ENOENT;
	return 0;
}

int
sgr_rewind ()
{
	if (! isopen) {
		errno = EINVAL;
		return 0;
	}
	sgr_cursor = 0;
	return 1;
}

struct sgrp *
sgr_next ()
{
	if (! isopen) {
		errno = EINVAL;
		return 0;
	}
	if (sgr_cursor == 0)
		sgr_cursor = __sgr_head;
	else
		sgr_cursor = sgr_cursor->sgr_next;

	while (sgr_cursor) {
		if (sgr_cursor->sgr_entry)
			return sgr_cursor->sgr_entry;

		sgr_cursor = sgr_cursor->sgr_next;
	}
	return 0;
}
