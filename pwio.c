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
 *	pw_lock				-- lock password file
 *	pw_open				-- logically open password file
 *	while transaction to process
 *		pw_(locate,update,remove) -- perform transaction
 *	done
 *	pw_close			-- commit transactions
 *	pw_unlock			-- remove password lock
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "pwd.h"
#include <stdio.h>

#ifdef	BSD
# include <strings.h>
#else
# include <string.h>
#endif

#ifndef lint
static	char	sccsid[] = "@(#)pwio.c	3.10	12:21:55	02 May 1993";
#endif

static	int	islocked;
static	int	isopen;
static	int	open_modes;
static	FILE	*pwfp;

struct	pw_file_entry {
	char	*pwf_line;
	int	pwf_changed;
	struct	passwd	*pwf_entry;
	struct	pw_file_entry *pwf_next;
};

struct	pw_file_entry	*__pwf_head;
static	struct	pw_file_entry	*pwf_tail;
static	struct	pw_file_entry	*pwf_cursor;
int	__pw_changed;
static	int	lock_pid;

#define	PW_LOCK	"/etc/passwd.lock"
#define	PW_TEMP "/etc/pwd.%d"
#define	PASSWD	"/etc/passwd"

static	char	pw_filename[BUFSIZ] = PASSWD;

extern	int	fputs();
extern	char	*fgets();
extern	char	*strdup();
extern	char	*malloc();
extern	struct	passwd	*sgetpwent();

/*
 * pw_dup - duplicate a password file entry
 *
 *	pw_dup() accepts a pointer to a password file entry and
 *	returns a pointer to a password file entry in allocated
 *	memory.
 */

static struct passwd *
pw_dup (pwent)
struct	passwd	*pwent;
{
	struct	passwd	*pw;

	if (! (pw = (struct passwd *) malloc (sizeof *pw)))
		return 0;

	if ((pw->pw_name = strdup (pwent->pw_name)) == 0 ||
			(pw->pw_passwd = strdup (pwent->pw_passwd)) == 0 ||
#ifdef	ATT_AGE
			(pw->pw_age = strdup (pwent->pw_age)) == 0 ||
#endif	/* ATT_AGE */
#ifdef	ATT_COMMENT
			(pw->pw_comment = strdup (pwent->pw_comment)) == 0 ||
#endif	/* ATT_COMMENT */
			(pw->pw_gecos = strdup (pwent->pw_gecos)) == 0 ||
			(pw->pw_dir = strdup (pwent->pw_dir)) == 0 ||
			(pw->pw_shell = strdup (pwent->pw_shell)) == 0)
		return 0;

	pw->pw_uid = pwent->pw_uid;
	pw->pw_gid = pwent->pw_gid;

	return pw;
}

/*
 * pw_free - free a dynamically allocated password file entry
 *
 *	pw_free() frees up the memory which was allocated for the
 *	pointed to entry.
 */

static void
pw_free (pwent)
struct	passwd	*pwent;
{
	free (pwent->pw_name);
	free (pwent->pw_passwd);
	free (pwent->pw_gecos);
	free (pwent->pw_dir);
	free (pwent->pw_shell);
}

/*
 * pw_name - change the name of the password file
 */

int
pw_name (name)
char	*name;
{
	if (isopen || strlen (name) > (BUFSIZ-10))
		return -1;

	strcpy (pw_filename, name);
	return 0;
}

/*
 * pw_lock - lock a password file
 *
 *	pw_lock() encapsulates the lock operation.  it returns
 *	TRUE or FALSE depending on the password file being
 *	properly locked.  the lock is set by creating a semaphore
 *	file, PW_LOCK.
 */

int
pw_lock ()
{
	int	fd;
	int	pid;
	int	len;
	char	file[BUFSIZ];
	char	lock[BUFSIZ];
	char	buf[32];
	struct	stat	sb;

	/*
	 * Quick check -- If I created this lock already, assume it is
	 * still there.
	 */

	if (islocked && lock_pid == getpid ())
		return 1;

	/*
	 * If we are using the "standard" password file, we create a
	 * well-known lock file.  Otherwise, we create one based on the
	 * name of the file being altered.
	 */

	if (strcmp (pw_filename, PASSWD) != 0) {
		sprintf (file, "%s.%d", pw_filename, lock_pid = getpid());
		sprintf (lock, "%s.lock", pw_filename);
	} else {
		sprintf (file, "%s.%d", PW_TEMP, lock_pid = getpid());
		strcpy (lock, PW_LOCK);
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
 * pw_unlock - logically unlock a password file
 *
 *	pw_unlock() removes the lock which was set by an earlier
 *	invocation of pw_lock().
 */

int
pw_unlock ()
{
	char	lock[BUFSIZ];

	/*
	 * If we are unlocking an open file, we aren't going to write
	 * out the contents.  This is the "abort" mechanism which allows
	 * all changes to be "aborted".
	 */

	if (isopen) {
		open_modes = O_RDONLY;
		if (! pw_close ())
			return 0;
	}

	/*
	 * If the file is locked, we reset some flags and remove the lock
	 * file.  But we must be the process which created the lock in the
	 * first place.  fork() can mess us up since it causes two processes
	 * to hold the lock.
	 */

  	if (islocked) {
  		islocked = 0;
		if (lock_pid != getpid ())
			return 0;

		strcpy (lock, pw_filename);
		strcat (lock, ".lock");
		(void) unlink (lock);
  		return 1;
	}
	return 0;
}

/*
 * pw_open - open a password file
 *
 *	pw_open() encapsulates the open operation.  it returns
 *	TRUE or FALSE depending on the password file being
 *	properly opened.
 */

int
pw_open (mode)
int	mode;
{
	char	buf[8192];
	char	*cp;
	struct	pw_file_entry	*pwf;
	struct	passwd	*pwent;

	if (isopen || (mode != O_RDONLY && mode != O_RDWR))
		return 0;

	if (mode != O_RDONLY && ! islocked &&
			strcmp (pw_filename, PASSWD) == 0)
		return 0;

	if ((pwfp = fopen (pw_filename, mode == O_RDONLY ? "r":"r+")) == 0)
		return 0;

	__pwf_head = pwf_tail = pwf_cursor = 0;
	__pw_changed = 0;

	while (fgets (buf, sizeof buf, pwfp) != (char *) 0) {
		if (cp = strrchr (buf, '\n'))
			*cp = '\0';

		if (! (pwf = (struct pw_file_entry *) malloc (sizeof *pwf)))
			return 0;

		pwf->pwf_changed = 0;
		pwf->pwf_line = strdup (buf);
		if ((pwent = sgetpwent (buf)) && ! (pwent = pw_dup (pwent)))
			return 0;

		pwf->pwf_entry = pwent;

		if (__pwf_head == 0) {
			__pwf_head = pwf_tail = pwf;
			pwf->pwf_next = 0;
		} else {
			pwf_tail->pwf_next = pwf;
			pwf->pwf_next = 0;
			pwf_tail = pwf;
		}
	}
	isopen++;
	open_modes = mode;

	return 1;
}

/*
 * pw_close - close the password file
 *
 *	pw_close() outputs any modified password file entries and
 *	frees any allocated memory.
 */

int
pw_close ()
{
	char	backup[BUFSIZ];
	int	mask;
	int	c;
	int	errors = 0;
	FILE	*bkfp;
	struct	pw_file_entry *pwf;
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
	strcpy (backup, pw_filename);
	strcat (backup, "-");

	if (open_modes == O_RDWR && __pw_changed) {
		mask = umask (0222);
		(void) unlink (backup);
		if ((bkfp = fopen (backup, "w")) == 0) {
			umask (mask);
			return 0;
		}
		umask (mask);
		fstat (fileno (pwfp), &sb);
		chown (backup, sb.st_uid, sb.st_gid);

		rewind (pwfp);
		while ((c = getc (pwfp)) != EOF) {
			if (putc (c, bkfp) == EOF) {
				fclose (bkfp);
				return 0;
			}
		}
		if (fclose (bkfp))
			return 0;

		isopen = 0;
		(void) fclose (pwfp);

		mask = umask (0222);
		if (! (pwfp = fopen (pw_filename, "w"))) {
			umask (mask);
			return 0;
		}
		umask (mask);

		for (pwf = __pwf_head;errors == 0 && pwf;pwf = pwf->pwf_next) {
			if (pwf->pwf_changed) {
				if (putpwent (pwf->pwf_entry, pwfp))
					errors++;
			} else {
				if (fputs (pwf->pwf_line, pwfp) == EOF)
					errors++;
				if (putc ('\n', pwfp) == EOF)
					errors++;
			}
		}
		if (fflush (pwfp))
			errors++;

		if (errors) {
			unlink (pw_filename);
			link (backup, pw_filename);
			unlink (backup);
			return 0;
		}
	}
	if (fclose (pwfp))
		return 0;

	pwfp = 0;

	while (__pwf_head != 0) {
		pwf = __pwf_head;
		__pwf_head = pwf->pwf_next;

		if (pwf->pwf_entry) {
			pw_free (pwf->pwf_entry);
			free (pwf->pwf_entry);
		}
		if (pwf->pwf_line)
			free (pwf->pwf_line);

		free (pwf);
	}
	pwf_tail = 0;
	isopen = 0;
	return 1;
}

int
pw_update (pwent)
struct	passwd	*pwent;
{
	struct	pw_file_entry	*pwf;
	struct	passwd	*npw;

	if (! isopen || open_modes == O_RDONLY) {
		errno = EINVAL;
		return 0;
	}
	for (pwf = __pwf_head;pwf != 0;pwf = pwf->pwf_next) {
		if (pwf->pwf_entry == 0)
			continue;

		if (strcmp (pwent->pw_name, pwf->pwf_entry->pw_name) != 0)
			continue;

		if (! (npw = pw_dup (pwent)))
			return 0;
		else {
			pw_free (pwf->pwf_entry);
			*(pwf->pwf_entry) = *npw;
		}
		pwf->pwf_changed = 1;
		pwf_cursor = pwf;
		return __pw_changed = 1;
	}
	pwf = (struct pw_file_entry *) malloc (sizeof *pwf);
	if (! (pwf->pwf_entry = pw_dup (pwent)))
		return 0;

	pwf->pwf_changed = 1;
	pwf->pwf_next = 0;
	pwf->pwf_line = 0;

	if (pwf_tail)
		pwf_tail->pwf_next = pwf;

	if (! __pwf_head)
		__pwf_head = pwf;

	pwf_tail = pwf;

	return __pw_changed = 1;
}

int
pw_remove (name)
char	*name;
{
	struct	pw_file_entry	*pwf;
	struct	pw_file_entry	*opwf;

	if (! isopen || open_modes == O_RDONLY) {
		errno = EINVAL;
		return 0;
	}
	for (opwf = 0, pwf = __pwf_head;pwf != 0;
			opwf = pwf, pwf = pwf->pwf_next) {
		if (! pwf->pwf_entry)
			continue;

		if (strcmp (name, pwf->pwf_entry->pw_name) != 0)
			continue;

		if (pwf == pwf_cursor)
			pwf_cursor = opwf;

		if (opwf != 0)
			opwf->pwf_next = pwf->pwf_next;
		else
			__pwf_head = pwf->pwf_next;

		if (pwf == pwf_tail)
			pwf_tail = opwf;

		return __pw_changed = 1;
	}
	errno = ENOENT;
	return 0;
}

struct passwd *
pw_locate (name)
char	*name;
{
	struct	pw_file_entry	*pwf;

	if (! isopen) {
		errno = EINVAL;
		return 0;
	}
	for (pwf = __pwf_head;pwf != 0;pwf = pwf->pwf_next) {
		if (pwf->pwf_entry == 0)
			continue;

		if (strcmp (name, pwf->pwf_entry->pw_name) == 0) {
			pwf_cursor = pwf;
			return pwf->pwf_entry;
		}
	}
	errno = ENOENT;
	return 0;
}

int
pw_rewind ()
{
	if (! isopen) {
		errno = EINVAL;
		return 0;
	}
	pwf_cursor = 0;
	return 1;
}

struct passwd *
pw_next ()
{
	if (! isopen) {
		errno = EINVAL;
		return 0;
	}
	if (pwf_cursor == 0)
		pwf_cursor = __pwf_head;
	else
		pwf_cursor = pwf_cursor->pwf_next;

	while (pwf_cursor) {
		if (pwf_cursor->pwf_entry)
			return pwf_cursor->pwf_entry;

		pwf_cursor = pwf_cursor->pwf_next;
	}
	return 0;
}
