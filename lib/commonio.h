/*
 * Copyright (c) 1990 - 1994, Julianne Frances Haugh
 * Copyright (c) 1996 - 2000, Marek Michałkiewicz
 * Copyright (c) 2001 - 2005, Tomasz Kłoczko
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

/* $Id$ */
#ifndef _COMMONIO_H
#define _COMMONIO_H

#ifdef WITH_SELINUX
#include <selinux/selinux.h>
#endif

#include "defines.h" /* bool */

/*
 * Linked list entry.
 */
struct commonio_entry {
	/*@null@*/char *line;
	/*@null@*/void *eptr;		/* struct passwd, struct spwd, ... */
	/*@dependent@*/ /*@null@*/struct commonio_entry *prev;
	/*@owned@*/ /*@null@*/struct commonio_entry *next;
	bool changed:1;
};

/*
 * Operations depending on database type: passwd, group, shadow etc.
 */
struct commonio_ops {
	/*
	 * Make a copy of the object (for example, struct passwd)
	 * and all strings pointed by it, in malloced memory.
	 */
	/*@null@*/ /*@only@*/void *(*dup) (const void *);

	/*
	 * free() the object including any strings pointed by it.
	 */
	void (*free) (/*@out@*/ /*@only@*/void *);

	/*
	 * Return the name of the object (for example, pw_name
	 * for struct passwd).
	 */
	const char *(*getname) (const void *);

	/*
	 * Parse a string, return object (in static area -
	 * should be copied using the dup operation above).
	 */
	void *(*parse) (const char *);

	/*
	 * Write the object to the file (this calls putpwent()
	 * for struct passwd, for example).
	 */
	int (*put) (const void *, FILE *);

	/*
	 * fgets and fputs (can be replaced by versions that
	 * understand line continuation conventions).
	 */
	/*@null@*/char *(*fgets) (/*@returned@*/ /*@out@*/char *s, int n, FILE *stream);
	int (*fputs) (const char *, FILE *);

	/*
	 * open_hook and close_hook.
	 * If non NULL, these functions will be called after the database
	 * is open or before it is closed.
	 * They return 0 on failure and 1 on success.
	 */
	/*@null@*/int (*open_hook) (void);
	/*@null@*/int (*close_hook) (void);
};

/*
 * Database structure.
 */
struct commonio_db {
	/*
	 * Name of the data file.
	 */
	char filename[1024];

	/*
	 * Operations from above.
	 */
	/*@observer@*/const struct commonio_ops *ops;

	/*
	 * Currently open file stream.
	 */
	/*@dependent@*/ /*@null@*/FILE *fp;

#ifdef WITH_SELINUX
	/*@null@*/security_context_t scontext;
#endif
	/*
	 * Head, tail, current position in linked list.
	 */
	/*@owned@*/ /*@null@*/struct commonio_entry *head, *tail, *cursor;

	/*
	 * Various flags.
	 */
	bool changed:1;
	bool isopen:1;
	bool locked:1;
	bool readonly:1;
};

extern int commonio_setname (struct commonio_db *, const char *);
extern bool commonio_present (const struct commonio_db *db);
extern int commonio_lock (struct commonio_db *);
extern int commonio_lock_nowait (struct commonio_db *);
extern int commonio_open (struct commonio_db *, int);
extern /*@observer@*/ /*@null@*/const void *commonio_locate (struct commonio_db *, const char *);
extern int commonio_update (struct commonio_db *, const void *);
extern int commonio_remove (struct commonio_db *, const char *);
extern int commonio_rewind (struct commonio_db *);
extern /*@observer@*/ /*@null@*/const void *commonio_next (struct commonio_db *);
extern int commonio_close (struct commonio_db *);
extern int commonio_unlock (struct commonio_db *);
extern void commonio_del_entry (struct commonio_db *,
				const struct commonio_entry *);
extern int commonio_sort_wrt (struct commonio_db *shadow,
			      struct commonio_db *passwd);
extern int commonio_sort (struct commonio_db *db,
			  int (*cmp) (const void *, const void *));

#endif
