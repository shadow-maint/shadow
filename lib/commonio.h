/* $Id: commonio.h,v 1.4 1998/01/29 23:22:27 marekm Exp $ */

/*
 * Linked list entry.
 */
struct commonio_entry {
	char *line;
	void *entry;  /* struct passwd, struct spwd, ... */
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
	void * (*dup) P_((const void *));

	/*
	 * free() the object including any strings pointed by it.
	 */
	void (*free) P_((void *));

	/*
	 * Return the name of the object (for example, pw_name
	 * for struct passwd).
	 */
	const char * (*getname) P_((const void *));

	/*
	 * Parse a string, return object (in static area -
	 * should be copied using the dup operation above).
	 */
	void * (*parse) P_((const char *));

	/*
	 * Write the object to the file (this calls putpwent()
	 * for struct passwd, for example).
	 */
	int (*put) P_((const void *, FILE *));

	/*
	 * fgets and fputs (can be replaced by versions that
	 * understand line continuation conventions).
	 */
	char * (*fgets) P_((char *, int, FILE *));
	int (*fputs) P_((const char *, FILE *));
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
	int use_lckpwdf:1;
};

extern int commonio_setname P_((struct commonio_db *, const char *));
extern int commonio_present P_((const struct commonio_db *));
extern int commonio_lock P_((struct commonio_db *));
extern int commonio_lock_nowait P_((struct commonio_db *));
extern int commonio_open P_((struct commonio_db *, int));
extern const void *commonio_locate P_((struct commonio_db *, const char *));
extern int commonio_update P_((struct commonio_db *, const void *));
extern int commonio_remove P_((struct commonio_db *, const char *));
extern int commonio_rewind P_((struct commonio_db *));
extern const void *commonio_next P_((struct commonio_db *));
extern int commonio_close P_((struct commonio_db *));
extern int commonio_unlock P_((struct commonio_db *));
extern void commonio_del_entry P_((struct commonio_db *, const struct commonio_entry *));

