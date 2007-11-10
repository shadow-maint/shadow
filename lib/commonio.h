/* $Id$ */

#ifdef WITH_SELINUX
#include <selinux/selinux.h>
#endif
/*
 * Linked list entry.
 */
struct commonio_entry {
	char *line;
	void *eptr;		/* struct passwd, struct spwd, ... */
	struct commonio_entry *prev, *next;
	int changed:1;
};

/*
 * Operations depending on database type: passwd, group, shadow etc.
 */
struct commonio_ops {
	/*
	 * Make a copy of the object (for example, struct passwd)
	 * and all strings pointed by it, in malloced memory.
	 */
	void *(*dup) (const void *);

	/*
	 * free() the object including any strings pointed by it.
	 */
	void (*free) (void *);

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
	char *(*fgets) (char *, int, FILE *);
	int (*fputs) (const char *, FILE *);
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
	struct commonio_ops *ops;

	/*
	 * Currently open file stream.
	 */
	FILE *fp;

#ifdef WITH_SELINUX
	security_context_t scontext;
#endif
	/*
	 * Head, tail, current position in linked list.
	 */
	struct commonio_entry *head, *tail, *cursor;

	/*
	 * Various flags.
	 */
	int changed:1;
	int isopen:1;
	int locked:1;
	int readonly:1;
};

extern int commonio_setname (struct commonio_db *, const char *);
extern int commonio_present (const struct commonio_db *);
extern int commonio_lock (struct commonio_db *);
extern int commonio_lock_nowait (struct commonio_db *);
extern int commonio_open (struct commonio_db *, int);
extern const void *commonio_locate (struct commonio_db *, const char *);
extern int commonio_update (struct commonio_db *, const void *);
extern int commonio_remove (struct commonio_db *, const char *);
extern int commonio_rewind (struct commonio_db *);
extern const void *commonio_next (struct commonio_db *);
extern int commonio_close (struct commonio_db *);
extern int commonio_unlock (struct commonio_db *);
extern void commonio_del_entry (struct commonio_db *,
				const struct commonio_entry *);
extern int commonio_sort_wrt (struct commonio_db *shadow,
			      struct commonio_db *passwd);
extern int commonio_sort (struct commonio_db *db,
			  int (*cmp) (const void *, const void *));
