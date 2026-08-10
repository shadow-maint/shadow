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
 *	This file implements a transaction oriented password database
 *	library.  The password file is updated one entry at a time.
 *	After each transaction the file must be logically closed and
 *	transferred to the existing password file.  The sequence of
 *	events is
 *
 *	spw_lock			-- lock shadow file
 *	spw_open			-- logically open shadow file
 *	while transaction to process
 *		spw_(locate,update,remove) -- perform transaction
 *	done
 *	spw_close			-- commit transactions
 *	spw_unlock			-- remove shadow lock
 */

#ifndef	lint
static	char	sccsid[] = "@(#)shadowio.c	3.7	07:56:13	06 May 1993";
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#ifdef	BSD
#include <strings.h>
#else
#include <string.h>
#endif
#include "shadow.h"

static	int	islocked;
static	int	isopen;
static	int	open_modes;
static	FILE	*spwfp;

struct	spw_file_entry {
	char	*spwf_line;
	int	spwf_changed;
	struct	spwd	*spwf_entry;
	struct	spw_file_entry *spwf_next;
};

struct	spw_file_entry	*__spwf_head;
static	struct	spw_file_entry	*spwf_tail;
static	struct	spw_file_entry	*spwf_cursor;
int	__sp_changed;
static	int	lock_pid;

#define	SPW_LOCK	"/etc/shadow.lock"
#define	SPW_TEMP	"/etc/spwd.%d"
#define	SHADOW		"/etc/shadow"

static	char	spw_filename[BUFSIZ] = SHADOW;

extern	char	*strdup();
extern	char	*malloc();
extern	struct	spwd	*sgetspent();

/*
 * spw_dup - duplicate a shadow file entry
 *
 *	spw_dup() accepts a pointer to a shadow file entry and
 *	returns a pointer to a shadow file entry in allocated
 *	memory.
 */

static struct spwd *
spw_dup (spwd)
struct	spwd	*spwd;
{
	struct	spwd	*spw;

	if (! (spw = (struct spwd *) malloc (sizeof *spw)))
		return 0;

	*spw = *spwd;
	if ((spw->sp_namp = strdup (spwd->sp_namp)) == 0 ||
			(spw->sp_pwdp = strdup (spwd->sp_pwdp)) == 0)
		return 0;

	return spw;
}

/*
 * spw_free - free a dynamically allocated shadow file entry
 *
 *	spw_free() frees up the memory which was allocated for the
 *	pointed to entry.
 */

static void
spw_free (spwd)
struct	spwd	*spwd;
{
	free (spwd->sp_namp);
	free (spwd->sp_pwdp);
}

/*
 * spw_name - change the name of the shadow password file
 */

int
spw_name (name)
char	*name;
{
	if (isopen || strlen (name) > (BUFSIZ-10))
		return -1;

	strcpy (spw_filename, name);
	return 0;
}

/*
 * spw_lock - lock a password file
 *
 *	spw_lock() encapsulates the lock operation.  it returns
 *	TRUE or FALSE depending on the password file being
 *	properly locked.  the lock is set by creating a semaphore
 *	file, SPW_LOCK.
 */

int
spw_lock ()
{
	int	fd;
	int	pid;
	int	len;
	char	file[BUFSIZ];
	char	lock[BUFSIZ];
	char	buf[32];
	struct	stat	sb;

	if (islocked)
		return 1;

	if (strcmp (spw_filename, SHADOW) != 0) {
		sprintf (file, "%s.%d", spw_filename, lock_pid = getpid ());
		sprintf (lock, "%s.lock", spw_filename);
	} else {
		sprintf (file, SPW_TEMP, lock_pid = getpid ());
		strcpy (lock, SPW_LOCK);
	}

	/*
	 * Create a lock file which can be switched into place
	 */

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

	if (link (file, lock) == 0) {
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

	if ((fd = open (lock, O_RDWR)) == -1 ||
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
	if (unlink (lock)) {
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

	if (link (file, lock) == 0) {
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
 * spw_unlock - logically unlock a shadow file
 *
 *	spw_unlock() removes the lock which was set by an earlier
 *	invocation of spw_lock().
 */

int
spw_unlock ()
{
	char	lock[BUFSIZ];

	if (isopen) {
		open_modes = O_RDONLY;
		if (! spw_close ())
			return 0;
	}
  	if (islocked) {
  		islocked = 0;
		if (lock_pid != getpid ())
			return 0;

		strcpy (lock, spw_filename);
		strcat (lock, ".lock");
		(void) unlink (lock);
		return 1;
	}
	return 0;
}

/*
 * spw_open - open a password file
 *
 *	spw_open() encapsulates the open operation.  it returns
 *	TRUE or FALSE depending on the shadow file being
 *	properly opened.
 */

int
spw_open (mode)
int	mode;
{
	char	buf[BUFSIZ];
	char	*cp;
	struct	spw_file_entry	*spwf;
	struct	spwd	*spwd;

	if (isopen || (mode != O_RDONLY && mode != O_RDWR))
		return 0;

	if (mode != O_RDONLY && ! islocked &&
			strcmp (spw_filename, SHADOW) == 0)
		return 0;

	if ((spwfp = fopen (spw_filename, mode == O_RDONLY ? "r":"r+")) == 0)
		return 0;

	__spwf_head = spwf_tail = spwf_cursor = 0;
	__sp_changed = 0;

	while (fgets (buf, sizeof buf, spwfp) != (char *) 0) {
		if (cp = strrchr (buf, '\n'))
			*cp = '\0';

		if (! (spwf = (struct spw_file_entry *) malloc (sizeof *spwf)))
			return 0;

		spwf->spwf_changed = 0;
		spwf->spwf_line = strdup (buf);
		if ((spwd = sgetspent (buf)) && ! (spwd = spw_dup (spwd)))
			return 0;

		spwf->spwf_entry = spwd;

		if (__spwf_head == 0) {
			__spwf_head = spwf_tail = spwf;
			spwf->spwf_next = 0;
		} else {
			spwf_tail->spwf_next = spwf;
			spwf->spwf_next = 0;
			spwf_tail = spwf;
		}
	}
	isopen++;
	open_modes = mode;

	return 1;
}

/*
 * spw_close - close the password file
 *
 *	spw_close() outputs any modified password file entries and
 *	frees any allocated memory.
 */

int
spw_close ()
{
	char	backup[BUFSIZ];
	int	mask;
	int	c;
	int	errors = 0;
	FILE	*bkfp;
	struct	spw_file_entry *spwf;
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
	strcpy (backup, spw_filename);
	strcat (backup, "-");

	if (open_modes == O_RDWR && __sp_changed) {
		mask = umask (0377);
		(void) unlink (backup);
		if ((bkfp = fopen (backup, "w")) == 0) {
			umask (mask);
			return 0;
		}
		umask (mask);
		(void) chmod (backup, 0400);
		fstat (fileno (spwfp), &sb);
		chown (backup, sb.st_uid, sb.st_gid);

		rewind (spwfp);
		while ((c = getc (spwfp)) != EOF) {
			if (putc (c, bkfp) == EOF) {
				fclose (bkfp);
				return 0;
			}
		}
		if (fclose (bkfp))
			return 0;

		isopen = 0;
		(void) fclose (spwfp);

		mask = umask (0377);
		if (! (spwfp = fopen (spw_filename, "w"))) {
			umask (mask);
			return 0;
		}
		umask (mask);

		for (spwf = __spwf_head;errors == 0 && spwf;
						spwf = spwf->spwf_next) {
			if (spwf->spwf_changed) {
				if (putspent (spwf->spwf_entry, spwfp))
					errors++;
			} else {
				if (fputs (spwf->spwf_line, spwfp) == EOF)
					errors++;
				if (putc ('\n', spwfp) == EOF)
					errors++;
			}
		}
		if (fflush (spwfp))
			errors++;

		if (errors) {
			unlink (spw_filename);
			link (backup, spw_filename);
			unlink (backup);
			return 0;
		}
	}
	if (fclose (spwfp))
		return 0;

	spwfp = 0;

	while (__spwf_head != 0) {
		spwf = __spwf_head;
		__spwf_head = spwf->spwf_next;

		if (spwf->spwf_entry) {
			spw_free (spwf->spwf_entry);
			free (spwf->spwf_entry);
		}
		if (spwf->spwf_line)
			free (spwf->spwf_line);

		free (spwf);
	}
	spwf_tail = 0;
	isopen = 0;
	return 1;
}

int
spw_update (spwd)
struct	spwd	*spwd;
{
	struct	spw_file_entry	*spwf;
	struct	spwd	*nspwd;

	if (! isopen || open_modes == O_RDONLY) {
		errno = EINVAL;
		return 0;
	}
	for (spwf = __spwf_head;spwf != 0;spwf = spwf->spwf_next) {
		if (spwf->spwf_entry == 0)
			continue;

		if (strcmp (spwd->sp_namp, spwf->spwf_entry->sp_namp) != 0)
			continue;

		if (! (nspwd = spw_dup (spwd)))
			return 0;
		else {
			spw_free (spwf->spwf_entry);
			*(spwf->spwf_entry) = *nspwd;
		}
		spwf->spwf_changed = 1;
		spwf_cursor = spwf;
		return __sp_changed = 1;
	}
	spwf = (struct spw_file_entry *) malloc (sizeof *spwf);
	if (! (spwf->spwf_entry = spw_dup (spwd)))
		return 0;

	spwf->spwf_changed = 1;
	spwf->spwf_next = 0;
	spwf->spwf_line = 0;

	if (spwf_tail)
		spwf_tail->spwf_next = spwf;

	if (! __spwf_head)
		__spwf_head = spwf;

	spwf_tail = spwf;

	return __sp_changed = 1;
}

int
spw_remove (name)
char	*name;
{
	struct	spw_file_entry	*spwf;
	struct	spw_file_entry	*ospwf;

	if (! isopen || open_modes == O_RDONLY) {
		errno = EINVAL;
		return 0;
	}
	for (ospwf = 0, spwf = __spwf_head;spwf != 0;
			ospwf = spwf, spwf = spwf->spwf_next) {
		if (! spwf->spwf_entry)
			continue;

		if (strcmp (name, spwf->spwf_entry->sp_namp) != 0)
			continue;

		if (spwf == spwf_cursor)
			spwf_cursor = ospwf;

		if (ospwf != 0)
			ospwf->spwf_next = spwf->spwf_next;
		else
			__spwf_head = spwf->spwf_next;

		if (spwf == spwf_tail)
			spwf_tail = ospwf;

		return __sp_changed = 1;
	}
	errno = ENOENT;
	return 0;
}

struct spwd *
spw_locate (name)
char	*name;
{
	struct	spw_file_entry	*spwf;

	if (! isopen) {
		errno = EINVAL;
		return 0;
	}
	for (spwf = __spwf_head;spwf != 0;spwf = spwf->spwf_next) {
		if (spwf->spwf_entry == 0)
			continue;

		if (strcmp (name, spwf->spwf_entry->sp_namp) == 0) {
			spwf_cursor = spwf;
			return spwf->spwf_entry;
		}
	}
	errno = ENOENT;
	return 0;
}

int
spw_rewind ()
{
	if (! isopen) {
		errno = EINVAL;
		return 0;
	}
	spwf_cursor = 0;
	return 1;
}

struct spwd *
spw_next ()
{
	if (! isopen) {
		errno = EINVAL;
		return 0;
	}
	if (spwf_cursor == 0)
		spwf_cursor = __spwf_head;
	else
		spwf_cursor = spwf_cursor->spwf_next;

	while (spwf_cursor) {
		if (spwf_cursor->spwf_entry)
			return spwf_cursor->spwf_entry;

		spwf_cursor = spwf_cursor->spwf_next;
	}
	return 0;
}
