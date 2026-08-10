/*
 * Copyright 1990, 1991, 1992, John F. Haugh II
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
 *	This file implements a transaction oriented group database
 *	library.  The group file is updated one entry at a time.
 *	After each transaction the file must be logically closed and
 *	transferred to the existing group file.  The sequence of
 *	events is
 *
 *	gr_lock				-- lock group file
 *	gr_open				-- logically open group file
 *	while transaction to process
 *		gr_(locate,update,remove) -- perform transaction
 *	done
 *	gr_close			-- commit transactions
 *	gr_unlock			-- remove group lock
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <grp.h>
#include <stdio.h>
#ifdef	BSD
#include <strings.h>
#else
#include <string.h>
#endif

#ifndef	lint
static	char	sccsid[] = "@(#)groupio.c	3.10 08:39:39 29 Apr 1993";
#endif

static	int	islocked;
static	int	isopen;
static	int	open_modes;
static	FILE	*grfp;

struct	gr_file_entry {
	char	*grf_line;
	int	grf_changed;
	struct	group	*grf_entry;
	struct	gr_file_entry *grf_next;
};

struct	gr_file_entry	*__grf_head;
static	struct	gr_file_entry	*grf_tail;
static	struct	gr_file_entry	*grf_cursor;
int	__gr_changed;
static	int	lock_pid;

#define	GR_LOCK	"/etc/group.lock"
#define	GR_TEMP "/etc/grp.%d"
#define	GROUP	"/etc/group"

static	char	gr_filename[BUFSIZ] = GROUP;

extern	char	*strdup();
extern	struct	group	*sgetgrent();
extern	char	*malloc();
extern	char	*fgetsx();

/*
 * gr_dup - duplicate a group file entry
 *
 *	gr_dup() accepts a pointer to a group file entry and
 *	returns a pointer to a group file entry in allocated
 *	memory.
 */

static struct group *
gr_dup (grent)
struct	group	*grent;
{
	struct	group	*gr;
	int	i;

	if (! (gr = (struct group *) malloc (sizeof *gr)))
		return 0;

	if ((gr->gr_name = strdup (grent->gr_name)) == 0 ||
			(gr->gr_passwd = strdup (grent->gr_passwd)) == 0)
		return 0;

	for (i = 0;grent->gr_mem[i];i++)
		;

	gr->gr_mem = (char **) malloc (sizeof (char *) * (i + 1));
	for (i = 0;grent->gr_mem[i];i++)
		if (! (gr->gr_mem[i] = strdup (grent->gr_mem[i])))
			return 0;

	gr->gr_mem[i] = 0;
	gr->gr_gid = grent->gr_gid;

	return gr;
}

/*
 * gr_free - free a dynamically allocated group file entry
 *
 *	gr_free() frees up the memory which was allocated for the
 *	pointed to entry.
 */

static void
gr_free (grent)
struct	group	*grent;
{
	int	i;

	free (grent->gr_name);
	free (grent->gr_passwd);

	for (i = 0;grent->gr_mem[i];i++)
		free (grent->gr_mem[i]);

	free ((char *) grent->gr_mem);
}

/*
 * gr_name - change the name of the group file
 */

int
gr_name (name)
char	*name;
{
	if (isopen || strlen (name) > (BUFSIZ-10))
		return -1;

	strcpy (gr_filename, name);
	return 0;
}

/*
 * gr_lock - lock a group file
 *
 *	gr_lock() encapsulates the lock operation.  it returns
 *	TRUE or FALSE depending on the group file being
 *	properly locked.  the lock is set by creating a semaphore
 *	file, GR_LOCK.
 */

int
gr_lock ()
{
	int	fd;
	int	pid;
	int	len;
	char	file[BUFSIZ];
	char	buf[32];
	struct	stat	sb;

	if (islocked)
		return 1;

	if (strcmp (gr_filename, GROUP) != 0)
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

	if (link (file, GR_LOCK) == 0) {
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

	if ((fd = open (GR_LOCK, O_RDWR)) == -1 ||
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
	if (unlink (GR_LOCK)) {
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

	if (link (file, GR_LOCK) == 0) {
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
 * gr_unlock - logically unlock a group file
 *
 *	gr_unlock() removes the lock which was set by an earlier
 *	invocation of gr_lock().
 */

int
gr_unlock ()
{
	if (isopen) {
		open_modes = O_RDONLY;
		if (! gr_close ())
			return 0;
	}
	if (islocked) {
		islocked = 0;
		if (lock_pid != getpid ())
			return 0;

		(void) unlink (GR_LOCK);
		return 1;
	}
	return 0;
}

/*
 * gr_open - open a group file
 *
 *	gr_open() encapsulates the open operation.  it returns
 *	TRUE or FALSE depending on the group file being
 *	properly opened.
 */

int
gr_open (mode)
int	mode;
{
	char	buf[8192];
	char	*cp;
	struct	gr_file_entry	*grf;
	struct	group	*grent;

	if (isopen || (mode != O_RDONLY && mode != O_RDWR))
		return 0;

	if (mode != O_RDONLY && ! islocked &&
			strcmp (gr_filename, GROUP) == 0)
		return 0;

	if ((grfp = fopen (gr_filename, mode == O_RDONLY ? "r":"r+")) == 0)
		return 0;

	__grf_head = grf_tail = grf_cursor = 0;
	__gr_changed = 0;

	while (fgetsx (buf, sizeof buf, grfp) != (char *) 0) {
		if (cp = strrchr (buf, '\n'))
			*cp = '\0';

		if (! (grf = (struct gr_file_entry *) malloc (sizeof *grf)))
			return 0;

		grf->grf_changed = 0;
		grf->grf_line = strdup (buf);
		if ((grent = sgetgrent (buf)) && ! (grent = gr_dup (grent)))
			return 0;

		grf->grf_entry = grent;

		if (__grf_head == 0) {
			__grf_head = grf_tail = grf;
			grf->grf_next = 0;
		} else {
			grf_tail->grf_next = grf;
			grf->grf_next = 0;
			grf_tail = grf;
		}
	}
	isopen++;
	open_modes = mode;

	return 1;
}

/*
 * gr_close - close the group file
 *
 *	gr_close() outputs any modified group file entries and
 *	frees any allocated memory.
 */

int
gr_close ()
{
	char	backup[BUFSIZ];
	int	mask;
	int	c;
	int	errors = 0;
	FILE	*bkfp;
	struct	gr_file_entry *grf;
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
	strcpy (backup, gr_filename);
	strcat (backup, "-");

	if (open_modes == O_RDWR && __gr_changed) {
		mask = umask (0222);
		(void) unlink (backup);
		if ((bkfp = fopen (backup, "w")) == 0) {
			umask (mask);
			return 0;
		}
		umask (mask);
		fstat (fileno (grfp), &sb);
		chown (backup, sb.st_uid, sb.st_gid);

		rewind (grfp);
		while ((c = getc (grfp)) != EOF) {
			if (putc (c, bkfp) == EOF) {
				fclose (bkfp);
				return 0;
			}
		}
		if (fclose (bkfp))
			return 0;

		isopen = 0;
		(void) fclose (grfp);

		mask = umask (0222);
		if (! (grfp = fopen (gr_filename, "w"))) {
			umask (mask);
			return 0;
		}
		umask (mask);

		for (grf = __grf_head;! errors && grf;grf = grf->grf_next) {
			if (grf->grf_changed) {
				if (putgrent (grf->grf_entry, grfp))
					errors++;
			} else {
				if (fputsx (grf->grf_line, grfp))
					errors++;

				if (putc ('\n', grfp) == EOF)
					errors++;
			}
		}
		if (fflush (grfp))
			errors++;

		if (errors) {
			unlink (gr_filename);
			link (backup, gr_filename);
			unlink (backup);
			return 0;
		}
	}
	if (fclose (grfp))
		return 0;

	grfp = 0;

	while (__grf_head != 0) {
		grf = __grf_head;
		__grf_head = grf->grf_next;

		if (grf->grf_entry) {
			gr_free (grf->grf_entry);
			free ((char *) grf->grf_entry);
		}
		if (grf->grf_line)
			free (grf->grf_line);

		free ((char *) grf);
	}
	grf_tail = 0;
	isopen = 0;
	return 1;
}

int
gr_update (grent)
struct	group	*grent;
{
	struct	gr_file_entry	*grf;
	struct	group	*ngr;

	if (! isopen || open_modes == O_RDONLY) {
		errno = EINVAL;
		return 0;
	}
	for (grf = __grf_head;grf != 0;grf = grf->grf_next) {
		if (grf->grf_entry == 0)
			continue;

		if (strcmp (grent->gr_name, grf->grf_entry->gr_name) != 0)
			continue;

		if (! (ngr = gr_dup (grent)))
			return 0;
		else {
			gr_free (grf->grf_entry);
			*(grf->grf_entry) = *ngr;
		}
		grf->grf_changed = 1;
		grf_cursor = grf;
		return __gr_changed = 1;
	}
	grf = (struct gr_file_entry *) malloc (sizeof *grf);
	if (! (grf->grf_entry = gr_dup (grent)))
		return 0;

	grf->grf_changed = 1;
	grf->grf_next = 0;
	grf->grf_line = 0;

	if (grf_tail)
		grf_tail->grf_next = grf;

	if (! __grf_head)
		__grf_head = grf;

	grf_tail = grf;

	return __gr_changed = 1;
}

int
gr_remove (name)
char	*name;
{
	struct	gr_file_entry	*grf;
	struct	gr_file_entry	*ogrf;

	if (! isopen || open_modes == O_RDONLY) {
		errno = EINVAL;
		return 0;
	}
	for (ogrf = 0, grf = __grf_head;grf != 0;
			ogrf = grf, grf = grf->grf_next) {
		if (! grf->grf_entry)
			continue;

		if (strcmp (name, grf->grf_entry->gr_name) != 0)
			continue;

		if (grf == grf_cursor)
			grf_cursor = ogrf;

		if (ogrf != 0)
			ogrf->grf_next = grf->grf_next;
		else
			__grf_head = grf->grf_next;

		if (grf == grf_tail)
			grf_tail = ogrf;

		return __gr_changed = 1;
	}
	errno = ENOENT;
	return 0;
}

struct group *
gr_locate (name)
char	*name;
{
	struct	gr_file_entry	*grf;

	if (! isopen) {
		errno = EINVAL;
		return 0;
	}
	for (grf = __grf_head;grf != 0;grf = grf->grf_next) {
		if (grf->grf_entry == 0)
			continue;

		if (strcmp (name, grf->grf_entry->gr_name) == 0) {
			grf_cursor = grf;
			return grf->grf_entry;
		}
	}
	errno = ENOENT;
	return 0;
}

int
gr_rewind ()
{
	if (! isopen) {
		errno = EINVAL;
		return 0;
	}
	grf_cursor = 0;
	return 1;
}

struct group *
gr_next ()
{
	if (! isopen) {
		errno = EINVAL;
		return 0;
	}
	if (grf_cursor == 0)
		grf_cursor = __grf_head;
	else
		grf_cursor = grf_cursor->grf_next;

	while (grf_cursor) {
		if (grf_cursor->grf_entry)
			return grf_cursor->grf_entry;

		grf_cursor = grf_cursor->grf_next;
	}
	return 0;
}
