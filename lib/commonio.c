/*
 * Copyright (c) 1990 - 1994, Julianne Frances Haugh
 * Copyright (c) 1996 - 2001, Marek Michałkiewicz
 * Copyright (c) 2001 - 2006, Tomasz Kłoczko
 * Copyright (c) 2007 - 2008, Nicolas François
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

#include "defines.h"
#include <sys/stat.h>
#include <stdlib.h>
#include <limits.h>
#include <utime.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include "nscd.h"
#ifdef WITH_SELINUX
#include <selinux/selinux.h>
static security_context_t old_context = NULL;
#endif
#include "commonio.h"

/* local function prototypes */
static int lrename (const char *, const char *);
static int check_link_count (const char *);
static int do_lock_file (const char *, const char *);
static FILE *fopen_set_perms (const char *, const char *, const struct stat *);
static int create_backup (const char *, FILE *);
static void free_linked_list (struct commonio_db *);
static void add_one_entry (struct commonio_db *, struct commonio_entry *);
static int name_is_nis (const char *);
static int write_all (const struct commonio_db *);
static struct commonio_entry *find_entry_by_name (struct commonio_db *,
						  const char *);
static struct commonio_entry *next_entry_by_name (struct commonio_db *,
                                                  struct commonio_entry *pos,
                                                  const char *);

static int lock_count = 0;
static int nscd_need_reload = 0;

/*
 * Simple rename(P) alternative that attempts to rename to symlink
 * target.
 */
int lrename (const char *old, const char *new)
{

	char resolved_path[PATH_MAX];
	int res;

#if defined(S_ISLNK)
	struct stat sb;
	if (lstat (new, &sb) == 0 && S_ISLNK (sb.st_mode)) {
		if (realpath (new, resolved_path) == NULL) {
			perror ("realpath in lrename()");
		} else {
			new = resolved_path;
		}
	}
#endif
	res = rename (old, new);
	return res;
}

static int check_link_count (const char *file)
{
	struct stat sb;

	if (stat (file, &sb) != 0)
		return 0;

	if (sb.st_nlink != 2)
		return 0;

	return 1;
}


static int do_lock_file (const char *file, const char *lock)
{
	int fd;
	int pid;
	int len;
	int retval;
	char buf[32];

	if ((fd = open (file, O_CREAT | O_EXCL | O_WRONLY, 0600)) == -1)
		return 0;

	pid = getpid ();
	snprintf (buf, sizeof buf, "%d", pid);
	len = strlen (buf) + 1;
	if (write (fd, buf, len) != len) {
		close (fd);
		unlink (file);
		return 0;
	}
	close (fd);

	if (link (file, lock) == 0) {
		retval = check_link_count (file);
		unlink (file);
		return retval;
	}

	if ((fd = open (lock, O_RDWR)) == -1) {
		unlink (file);
		errno = EINVAL;
		return 0;
	}
	len = read (fd, buf, sizeof (buf) - 1);
	close (fd);
	if (len <= 0) {
		unlink (file);
		errno = EINVAL;
		return 0;
	}
	buf[len] = '\0';
	if ((pid = strtol (buf, (char **) 0, 10)) == 0) {
		unlink (file);
		errno = EINVAL;
		return 0;
	}
	if (kill (pid, 0) == 0) {
		unlink (file);
		errno = EEXIST;
		return 0;
	}
	if (unlink (lock) != 0) {
		unlink (file);
		return 0;
	}

	retval = 0;
	if (link (file, lock) == 0 && check_link_count (file))
		retval = 1;

	unlink (file);
	return retval;
}


static FILE *fopen_set_perms (const char *name, const char *mode,
			      const struct stat *sb)
{
	FILE *fp;
	mode_t mask;

	mask = umask (0777);
	fp = fopen (name, mode);
	umask (mask);
	if (!fp)
		return NULL;

#ifdef HAVE_FCHOWN
	if (fchown (fileno (fp), sb->st_uid, sb->st_gid))
		goto fail;
#else
	if (chown (name, sb->st_mode))
		goto fail;
#endif

#ifdef HAVE_FCHMOD
	if (fchmod (fileno (fp), sb->st_mode & 0664))
		goto fail;
#else
	if (chmod (name, sb->st_mode & 0664))
		goto fail;
#endif
	return fp;

      fail:
	fclose (fp);
	unlink (name);
	return NULL;
}


static int create_backup (const char *backup, FILE * fp)
{
	struct stat sb;
	struct utimbuf ub;
	FILE *bkfp;
	int c;
	mode_t mask;

	if (fstat (fileno (fp), &sb))
		return -1;

	mask = umask (077);
	bkfp = fopen (backup, "w");
	umask (mask);
	if (!bkfp)
		return -1;

	/* TODO: faster copy, not one-char-at-a-time.  --marekm */
	c = 0;
	if (fseek (fp, 0, SEEK_SET) == 0)
		while ((c = getc (fp)) != EOF) {
			if (putc (c, bkfp) == EOF)
				break;
		}
	if (c != EOF || ferror (fp) || fflush (bkfp)) {
		fclose (bkfp);
		return -1;
	}
	if (fclose (bkfp))
		return -1;

	ub.actime = sb.st_atime;
	ub.modtime = sb.st_mtime;
	utime (backup, &ub);
	return 0;
}


static void free_linked_list (struct commonio_db *db)
{
	struct commonio_entry *p;

	while (db->head) {
		p = db->head;
		db->head = p->next;

		if (p->line)
			free (p->line);

		if (p->eptr)
			db->ops->free (p->eptr);

		free (p);
	}
	db->tail = NULL;
}


int commonio_setname (struct commonio_db *db, const char *name)
{
	snprintf (db->filename, sizeof (db->filename), "%s", name);
	return 1;
}


int commonio_present (const struct commonio_db *db)
{
	return (access (db->filename, F_OK) == 0);
}


int commonio_lock_nowait (struct commonio_db *db)
{
	char file[1024];
	char lock[1024];

	if (db->locked)
		return 1;

	snprintf (file, sizeof file, "%s.%ld", db->filename, (long) getpid ());
	snprintf (lock, sizeof lock, "%s.lock", db->filename);
	if (do_lock_file (file, lock)) {
		db->locked = 1;
		lock_count++;
		return 1;
	}
	return 0;
}


int commonio_lock (struct commonio_db *db)
{
#ifdef HAVE_LCKPWDF
	/*
	 * only if the system libc has a real lckpwdf() - the one from
	 * lockpw.c calls us and would cause infinite recursion!
	 */

	/*
	 * Call lckpwdf() on the first lock.
	 * If it succeeds, call *_lock() only once
	 * (no retries, it should always succeed).
	 */
	if (lock_count == 0) {
		if (lckpwdf () == -1)
			return 0;	/* failure */
	}

	if (commonio_lock_nowait (db))
		return 1;	/* success */

	ulckpwdf ();
	return 0;		/* failure */
#else
	int i;

	/*
	 * lckpwdf() not used - do it the old way.
	 */
#ifndef LOCK_TRIES
#define LOCK_TRIES 15
#endif

#ifndef LOCK_SLEEP
#define LOCK_SLEEP 1
#endif
	for (i = 0; i < LOCK_TRIES; i++) {
		if (i > 0)
			sleep (LOCK_SLEEP);	/* delay between retries */
		if (commonio_lock_nowait (db))
			return 1;	/* success */
		/* no unnecessary retries on "permission denied" errors */
		if (geteuid () != 0)
			return 0;
	}
	return 0;		/* failure */
#endif
}

static void dec_lock_count (void)
{
	if (lock_count > 0) {
		lock_count--;
		if (lock_count == 0) {
			/* Tell nscd when lock count goes to zero,
			   if any of the files were changed.  */
			if (nscd_need_reload) {
				nscd_flush_cache ("passwd");
				nscd_flush_cache ("group");
				nscd_need_reload = 0;
			}
#ifdef HAVE_LCKPWDF
			ulckpwdf ();
#endif
		}
	}
}


int commonio_unlock (struct commonio_db *db)
{
	char lock[1024];

	if (db->isopen) {
		db->readonly = 1;
		if (!commonio_close (db)) {
			if (db->locked)
				dec_lock_count ();
			return 0;
		}
	}
	if (db->locked) {
		/*
		 * Unlock in reverse order: remove the lock file,
		 * then call ulckpwdf() (if used) on last unlock.
		 */
		db->locked = 0;
		snprintf (lock, sizeof lock, "%s.lock", db->filename);
		unlink (lock);
		dec_lock_count ();
		return 1;
	}
	return 0;
}


static void add_one_entry (struct commonio_db *db, struct commonio_entry *p)
{
	p->next = NULL;
	p->prev = db->tail;
	if (!db->head)
		db->head = p;
	if (db->tail)
		db->tail->next = p;
	db->tail = p;
}


static int name_is_nis (const char *n)
{
	return (n[0] == '+' || n[0] == '-');
}


/*
 * New entries are inserted before the first NIS entry.  Order is preserved
 * when db is written out.
 */
#ifndef KEEP_NIS_AT_END
#define KEEP_NIS_AT_END 1
#endif

#if KEEP_NIS_AT_END
static void add_one_entry_nis (struct commonio_db *, struct commonio_entry *);

/*
 * Insert an entry between the regular entries, and the NIS entries.
 */
static void
add_one_entry_nis (struct commonio_db *db, struct commonio_entry *newp)
{
	struct commonio_entry *p;

	for (p = db->head; p; p = p->next) {
		if (name_is_nis
		    (p->eptr ? db->ops->getname (p->eptr) : p->line)) {
			newp->next = p;
			newp->prev = p->prev;
			if (p->prev)
				p->prev->next = newp;
			else
				db->head = newp;
			p->prev = newp;
			return;
		}
	}
	add_one_entry (db, newp);
}
#endif				/* KEEP_NIS_AT_END */

/* Initial buffer size, as well as increment if not sufficient
   (for reading very long lines in group files).  */
#define BUFLEN 4096

int commonio_open (struct commonio_db *db, int mode)
{
	char *buf;
	char *cp;
	char *line;
	struct commonio_entry *p;
	void *eptr;
	int flags = mode;
	int buflen;
	int saved_errno;

	mode &= ~O_CREAT;

	if (db->isopen || (mode != O_RDONLY && mode != O_RDWR)) {
		errno = EINVAL;
		return 0;
	}
	db->readonly = (mode == O_RDONLY);
	if (!db->readonly && !db->locked) {
		errno = EACCES;
		return 0;
	}

	db->head = db->tail = db->cursor = NULL;
	db->changed = 0;

	db->fp = fopen (db->filename, db->readonly ? "r" : "r+");

	/*
	 * If O_CREAT was specified and the file didn't exist, it will be
	 * created by commonio_close().  We have no entries to read yet.  --marekm
	 */
	if (!db->fp) {
		if ((flags & O_CREAT) && errno == ENOENT) {
			db->isopen = 1;
			return 1;
		}
		return 0;
	}

	/* Do not inherit fd in spawned processes (e.g. nscd) */
	fcntl(fileno(db->fp), F_SETFD, FD_CLOEXEC);

#ifdef WITH_SELINUX
	db->scontext = NULL;
	if ((is_selinux_enabled () > 0) && (!db->readonly)) {
		if (fgetfilecon (fileno (db->fp), &db->scontext) < 0) {
			goto cleanup_errno;
		}
	}
#endif

	buflen = BUFLEN;
	buf = (char *) malloc (buflen);
	if (!buf)
		goto cleanup_ENOMEM;

	while (db->ops->fgets (buf, buflen, db->fp)) {
		while (!(cp = strrchr (buf, '\n')) && !feof (db->fp)) {
			int len;

			buflen += BUFLEN;
			cp = (char *) realloc (buf, buflen);
			if (!cp)
				goto cleanup_buf;
			buf = cp;
			len = strlen (buf);
			db->ops->fgets (buf + len, buflen - len, db->fp);
		}
		if ((cp = strrchr (buf, '\n')))
			*cp = '\0';

		if (!(line = strdup (buf)))
			goto cleanup_buf;

		if (name_is_nis (line)) {
			eptr = NULL;
		} else if ((eptr = db->ops->parse (line))) {
			eptr = db->ops->dup (eptr);
			if (!eptr)
				goto cleanup_line;
		}

		p = (struct commonio_entry *) malloc (sizeof *p);
		if (!p)
			goto cleanup_entry;

		p->eptr = eptr;
		p->line = line;
		p->changed = 0;

		add_one_entry (db, p);
	}

	free (buf);

	if (ferror (db->fp))
		goto cleanup_errno;

	if (db->ops->open_hook && !db->ops->open_hook ())
		goto cleanup_errno;

	db->isopen = 1;
	return 1;

      cleanup_entry:
	if (eptr)
		db->ops->free (eptr);
      cleanup_line:
	free (line);
      cleanup_buf:
	free (buf);
      cleanup_ENOMEM:
	errno = ENOMEM;
      cleanup_errno:
	saved_errno = errno;
	free_linked_list (db);
#ifdef WITH_SELINUX
	if (db->scontext != NULL) {
		freecon (db->scontext);
		db->scontext = NULL;
	}
#endif
	fclose (db->fp);
	db->fp = NULL;
	errno = saved_errno;
	return 0;
}

/*
 * Sort given db according to cmp function (usually compares uids)
 */
int
commonio_sort (struct commonio_db *db, int (*cmp) (const void *, const void *))
{
	struct commonio_entry **entries, *ptr;
	int n = 0, i;

	for (ptr = db->head; ptr; ptr = ptr->next)
		n++;

	if (n <= 1)
		return 0;

	entries = malloc (n * sizeof (struct commonio_entry *));
	if (entries == NULL)
		return -1;

	n = 0;
	for (ptr = db->head; ptr; ptr = ptr->next)
		entries[n++] = ptr;
	qsort (entries, n, sizeof (struct commonio_entry *), cmp);

	db->head = entries[0];
	db->tail = entries[--n];
	db->head->prev = NULL;
	db->head->next = entries[1];
	db->tail->prev = entries[n - 1];
	db->tail->next = NULL;

	for (i = 1; i < n; i++) {
		entries[i]->prev = entries[i - 1];
		entries[i]->next = entries[i + 1];
	}

	free (entries);
	db->changed = 1;

	return 0;
}

/*
 * Sort entries in db according to order in another.
 */
int commonio_sort_wrt (struct commonio_db *shadow, struct commonio_db *passwd)
{
	struct commonio_entry *head = NULL, *pw_ptr, *spw_ptr;
	const char *name;

	if (!shadow || !shadow->head)
		return 0;

	for (pw_ptr = passwd->head; pw_ptr; pw_ptr = pw_ptr->next) {
		if (pw_ptr->eptr == NULL)
			continue;
		name = passwd->ops->getname (pw_ptr->eptr);
		for (spw_ptr = shadow->head; spw_ptr; spw_ptr = spw_ptr->next)
			if (strcmp (name, shadow->ops->getname (spw_ptr->eptr))
			    == 0)
				break;
		if (spw_ptr == NULL)
			continue;
		commonio_del_entry (shadow, spw_ptr);
		spw_ptr->next = head;
		head = spw_ptr;
	}

	for (spw_ptr = head; spw_ptr; spw_ptr = head) {
		head = head->next;

		if (shadow->head)
			shadow->head->prev = spw_ptr;
		spw_ptr->next = shadow->head;
		shadow->head = spw_ptr;
	}

	shadow->head->prev = NULL;
	shadow->changed = 1;

	return 0;
}

/*
 * write_all - Write the database to its file.
 *
 * It returns 0 if all the entries could be writen correctly.
 */
static int write_all (const struct commonio_db *db)
{
	const struct commonio_entry *p;
	void *eptr;

	for (p = db->head; p; p = p->next) {
		if (p->changed) {
			eptr = p->eptr;
			if (db->ops->put (eptr, db->fp))
				return -1;
		} else if (p->line) {
			if (db->ops->fputs (p->line, db->fp) == EOF)
				return -1;
			if (putc ('\n', db->fp) == EOF)
				return -1;
		}
	}
	return 0;
}


int commonio_close (struct commonio_db *db)
{
	char buf[1024];
	int errors = 0;
	struct stat sb;

	if (!db->isopen) {
		errno = EINVAL;
		return 0;
	}
	db->isopen = 0;

	if (!db->changed || db->readonly) {
		fclose (db->fp);
		db->fp = NULL;
		goto success;
	}

	if (db->ops->close_hook && !db->ops->close_hook ())
		goto fail;

	memzero (&sb, sizeof sb);
	if (db->fp) {
		if (fstat (fileno (db->fp), &sb)) {
			fclose (db->fp);
			db->fp = NULL;
			goto fail;
		}
#ifdef WITH_SELINUX
		if (db->scontext != NULL) {
			if (getfscreatecon (&old_context) < 0) {
				errors++;
				goto fail;
			}
			if (setfscreatecon (db->scontext) < 0) {
				errors++;
				goto fail;
			}
		}
#endif
		/*
		 * Create backup file.
		 */
		snprintf (buf, sizeof buf, "%s-", db->filename);

		if (create_backup (buf, db->fp))
			errors++;

		if (fclose (db->fp))
			errors++;

		if (errors) {
			db->fp = NULL;
			goto fail;
		}
	} else {
		/*
		 * Default permissions for new [g]shadow files.
		 * (passwd and group always exist...)
		 */
		sb.st_mode = 0400;
		sb.st_uid = 0;
		sb.st_gid = 0;
	}

	snprintf (buf, sizeof buf, "%s+", db->filename);

	db->fp = fopen_set_perms (buf, "w", &sb);
	if (!db->fp)
		goto fail;

	if (write_all (db))
		errors++;

	if (fflush (db->fp))
		errors++;
#ifdef HAVE_FSYNC
	if (fsync (fileno (db->fp)))
		errors++;
#else
	sync ();
#endif
	if (fclose (db->fp))
		errors++;

	db->fp = NULL;

	if (errors) {
		unlink (buf);
		goto fail;
	}

	if (lrename (buf, db->filename))
		goto fail;

	nscd_need_reload = 1;
	goto success;
      fail:
	errors++;
      success:

#ifdef WITH_SELINUX
	if (db->scontext != NULL) {
		if (setfscreatecon (old_context) < 0) {
			errors++;
		}
		if (old_context != NULL) {
			freecon (old_context);
			old_context = NULL;
		}
		freecon (db->scontext);
		db->scontext = NULL;
	}
#endif
	free_linked_list (db);
	return errors == 0;
}

static struct commonio_entry *next_entry_by_name (struct commonio_db *db,
                                                  struct commonio_entry *pos,
                                                  const char *name)
{
	struct commonio_entry *p;
	void *ep;

	if (NULL == pos)
		return NULL;

	for (p = pos; p; p = p->next) {
		ep = p->eptr;
		if (ep && strcmp (db->ops->getname (ep), name) == 0)
			break;
	}
	return p;
}

static struct commonio_entry *find_entry_by_name (struct commonio_db *db,
						  const char *name)
{
	return next_entry_by_name(db, db->head, name);
}


int commonio_update (struct commonio_db *db, const void *eptr)
{
	struct commonio_entry *p;
	void *nentry;

	if (!db->isopen || db->readonly) {
		errno = EINVAL;
		return 0;
	}
	if (!(nentry = db->ops->dup (eptr))) {
		errno = ENOMEM;
		return 0;
	}
	p = find_entry_by_name (db, db->ops->getname (eptr));
	if (p) {
		if (next_entry_by_name (db, p->next, db->ops->getname (eptr)))
		{
			fprintf (stderr, _("Multiple entries named '%s' in %s. Please fix this with pwck or grpck.\n"), db->ops->getname (eptr), db->filename);
			return 0;
		}
		db->ops->free (p->eptr);
		p->eptr = nentry;
		p->changed = 1;
		db->cursor = p;

		db->changed = 1;
		return 1;
	}
	/* not found, new entry */
	p = (struct commonio_entry *) malloc (sizeof *p);
	if (!p) {
		db->ops->free (nentry);
		errno = ENOMEM;
		return 0;
	}

	p->eptr = nentry;
	p->line = NULL;
	p->changed = 1;

#if KEEP_NIS_AT_END
	add_one_entry_nis (db, p);
#else
	add_one_entry (db, p);
#endif

	db->changed = 1;
	return 1;
}


void commonio_del_entry (struct commonio_db *db, const struct commonio_entry *p)
{
	if (p == db->cursor)
		db->cursor = p->next;

	if (p->prev)
		p->prev->next = p->next;
	else
		db->head = p->next;

	if (p->next)
		p->next->prev = p->prev;
	else
		db->tail = p->prev;

	db->changed = 1;
}

/*
 * commonio_remove - Remove the entry of the given name from the database.
 */
int commonio_remove (struct commonio_db *db, const char *name)
{
	struct commonio_entry *p;

	if (!db->isopen || db->readonly) {
		errno = EINVAL;
		return 0;
	}
	p = find_entry_by_name (db, name);
	if (!p) {
		errno = ENOENT;
		return 0;
	}
	if (next_entry_by_name (db, p->next, name)) {
		fprintf (stderr, _("Multiple entries named '%s' in %s. Please fix this with pwck or grpck.\n"), name, db->filename);
		return 0;
	}

	commonio_del_entry (db, p);

	if (p->line)
		free (p->line);

	if (p->eptr)
		db->ops->free (p->eptr);

	return 1;
}

/*
 * commonio_locate - Find the first entry with the specified name in
 *                   the database.
 *
 *	If found, it returns the entry and set the cursor of the database to
 *	that entry.
 *
 *	Otherwise, it returns NULL.
 */
const void *commonio_locate (struct commonio_db *db, const char *name)
{
	struct commonio_entry *p;

	if (!db->isopen) {
		errno = EINVAL;
		return NULL;
	}
	p = find_entry_by_name (db, name);
	if (!p) {
		errno = ENOENT;
		return NULL;
	}
	db->cursor = p;
	return p->eptr;
}

/*
 * commonio_rewind - Restore the database cursor to the first entry.
 *
 * It returns 0 on error, 1 on success.
 */
int commonio_rewind (struct commonio_db *db)
{
	if (!db->isopen) {
		errno = EINVAL;
		return 0;
	}
	db->cursor = NULL;
	return 1;
}

/*
 * commonio_next - Return the next entry of the specified database
 *
 * It returns the next entry, or NULL if no other entries could be found.
 */
const void *commonio_next (struct commonio_db *db)
{
	void *eptr;

	if (!db->isopen) {
		errno = EINVAL;
		return 0;
	}
	if (db->cursor == NULL)
		db->cursor = db->head;
	else
		db->cursor = db->cursor->next;

	while (db->cursor) {
		eptr = db->cursor->eptr;
		if (eptr)
			return eptr;

		db->cursor = db->cursor->next;
	}
	return NULL;
}
