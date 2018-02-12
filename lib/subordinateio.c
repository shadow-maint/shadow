/*
 * Copyright (c) 2012 - Eric Biederman
 */

#include <config.h>

#ifdef ENABLE_SUBIDS

#include "prototypes.h"
#include "defines.h"
#include <stdio.h>
#include "commonio.h"
#include "subordinateio.h"
#include <sys/types.h>
#include <pwd.h>

struct subordinate_range {
	const char *owner;
	unsigned long start;
	unsigned long count;
};

#define NFIELDS 3

/*
 * subordinate_dup: create a duplicate range
 *
 * @ent: a pointer to a subordinate_range struct
 *
 * Returns a pointer to a newly allocated duplicate subordinate_range struct
 * or NULL on failure
 */
static /*@null@*/ /*@only@*/void *subordinate_dup (const void *ent)
{
	const struct subordinate_range *rangeent = ent;
	struct subordinate_range *range;

	range = (struct subordinate_range *) malloc (sizeof *range);
	if (NULL == range) {
		return NULL;
	}
	range->owner = strdup (rangeent->owner);
	if (NULL == range->owner) {
		free(range);
		return NULL;
	}
	range->start = rangeent->start;
	range->count = rangeent->count;

	return range;
}

/*
 * subordinate_free: free a subordinate_range struct
 *
 * @ent: pointer to a subordinate_range struct to free.
 */
static void subordinate_free (/*@out@*/ /*@only@*/void *ent)
{
	struct subordinate_range *rangeent = ent;
	
	free ((void *)(rangeent->owner));
	free (rangeent);
}

/*
 * subordinate_parse:
 *
 * @line: a line to parse
 *
 * Returns a pointer to a subordinate_range struct representing the values
 * in @line, or NULL on failure.  Note that the returned value should not
 * be freed by the caller.
 */
static void *subordinate_parse (const char *line)
{
	static struct subordinate_range range;
	static char rangebuf[1024];
	int i;
	char *cp;
	char *fields[NFIELDS];

	/*
	 * Copy the string to a temporary buffer so the substrings can
	 * be modified to be NULL terminated.
	 */
	if (strlen (line) >= sizeof rangebuf)
		return NULL;	/* fail if too long */
	strcpy (rangebuf, line);

	/*
	 * Save a pointer to the start of each colon separated
	 * field.  The fields are converted into NUL terminated strings.
	 */

	for (cp = rangebuf, i = 0; (i < NFIELDS) && (NULL != cp); i++) {
		fields[i] = cp;
		while (('\0' != *cp) && (':' != *cp)) {
			cp++;
		}

		if ('\0' != *cp) {
			*cp = '\0';
			cp++;
		} else {
			cp = NULL;
		}
	}

	/*
	 * There must be exactly NFIELDS colon separated fields or
	 * the entry is invalid.  Also, fields must be non-blank.
	 */
	if (i != NFIELDS || *fields[0] == '\0' || *fields[1] == '\0' || *fields[2] == '\0')
		return NULL;
	range.owner = fields[0];
	if (getulong (fields[1], &range.start) == 0)
		return NULL;
	if (getulong (fields[2], &range.count) == 0)
		return NULL;

	return &range;
}

/*
 * subordinate_put: print a subordinate_range value to a file
 *
 * @ent: a pointer to a subordinate_range struct to print out.
 * @file: file to which to print.
 *
 * Returns 0 on success, -1 on error.
 */
static int subordinate_put (const void *ent, FILE * file)
{
	const struct subordinate_range *range = ent;

	return fprintf(file, "%s:%lu:%lu\n",
			       range->owner,
			       range->start,
			       range->count) < 0 ? -1  : 0;
}

static struct commonio_ops subordinate_ops = {
	subordinate_dup,	/* dup */
	subordinate_free,	/* free */
	NULL,			/* getname */
	subordinate_parse,	/* parse */
	subordinate_put,	/* put */
	fgets,			/* fgets */
	fputs,			/* fputs */
	NULL,			/* open_hook */
	NULL,			/* close_hook */
};

static /*@observer@*/ /*@null*/const struct subordinate_range *subordinate_next(struct commonio_db *db)
{
	return (const struct subordinate_range *)commonio_next (db);
}

/*
 * range_exists: Check whether @owner owns any ranges
 *
 * @db: database to query
 * @owner: owner being queried
 *
 * Returns true if @owner owns any subuid ranges, false otherwise.
 */
static const bool range_exists(struct commonio_db *db, const char *owner)
{
	const struct subordinate_range *range;
	commonio_rewind(db);
	while ((range = commonio_next(db)) != NULL) {
		if (0 == strcmp(range->owner, owner))
			return true;
	}
	return false;
}

/*
 * find_range: find a range which @owner is authorized to use which includes
 *             subuid @val.
 *
 * @db: database to query
 * @owner: owning uid being queried
 * @val: subuid being searched for.
 *
 * Returns a range of subuids belonging to @owner and including the subuid
 * @val, or NULL if no such range exists.
 */
static const struct subordinate_range *find_range(struct commonio_db *db,
						  const char *owner, unsigned long val)
{
	const struct subordinate_range *range;

	/*
	 * Search for exact username/group specification
	 *
	 * This is the original method - go fast through the db, doing only
	 * exact username/group string comparison. Therefore we leave it as-is
	 * for the time being, in order to keep it equally fast as it was
	 * before.
	 */
	commonio_rewind(db);
	while ((range = commonio_next(db)) != NULL) {
		unsigned long first = range->start;
		unsigned long last = first + range->count - 1;

		if (0 != strcmp(range->owner, owner))
			continue;

		if ((val >= first) && (val <= last))
			return range;
	}


        /*
         * We only do special handling for these two files
         */
        if ((0 != strcmp(db->filename, "/etc/subuid")) && (0 != strcmp(db->filename, "/etc/subgid")))
                return NULL;

        /*
         * Search loop above did not produce any result. Let's rerun it,
         * but this time try to match actual UIDs. The first entry that
         * matches is considered a success.
         * (It may be specified as literal UID or as another username which
         * has the same UID as the username we are looking for.)
         */
        struct passwd *pwd;
        uid_t          owner_uid;
        char           owner_uid_string[33] = "";


        /* Get UID of the username we are looking for */
        pwd = getpwnam(owner);
        if (NULL == pwd) {
                /* Username not defined in /etc/passwd, or error occured during lookup */
                return NULL;
        }
        owner_uid = pwd->pw_uid;
        sprintf(owner_uid_string, "%lu", (unsigned long int)owner_uid);

        commonio_rewind(db);
        while ((range = commonio_next(db)) != NULL) {
                unsigned long first = range->start;
                unsigned long last = first + range->count - 1;

                /* For performance reasons check range before using getpwnam() */
                if ((val < first) || (val > last)) {
                        continue;
                }

                /*
                 * Range matches. Check if range owner is specified
                 * as numeric UID and if it matches.
                 */
                if (0 == strcmp(range->owner, owner_uid_string)) {
                        return range;
                }

                /*
                 * Ok, this range owner is not specified as numeric UID
                 * we are looking for. It may be specified as another
                 * UID or as a literal username.
                 *
                 * If specified as another UID, the call to getpwnam()
                 * will return NULL.
                 *
                 * If specified as literal username, we will get its
                 * UID and compare that to UID we are looking for.
                 */
                const struct passwd *range_owner_pwd;

                range_owner_pwd = getpwnam(range->owner);
                if (NULL == range_owner_pwd) {
                        continue;
                }

                if (owner_uid == range_owner_pwd->pw_uid) {
                        return range;
                }
        }

	return NULL;
}

/*
 * have_range: check whether @owner is authorized to use the range
 *             (@start .. @start+@count-1).
 * @db: database to check
 * @owner: owning uid being queried
 * @start: start of range
 * @count: number of uids in range
 *
 * Returns true if @owner is authorized to use the range, false otherwise.
 */
static bool have_range(struct commonio_db *db,
		       const char *owner, unsigned long start, unsigned long count)
{
	const struct subordinate_range *range;
	unsigned long end;

	if (count == 0)
		return false;

	end = start + count - 1;
	range = find_range (db, owner, start);
	while (range) {
		unsigned long last; 

		last = range->start + range->count - 1;
		if (last >= (start + count - 1))
			return true;

		count = end - last;
		start = last + 1;
		range = find_range(db, owner, start);
	}
	return false;
}

/*
 * subordinate_range_cmp: compare uid ranges
 *
 * @p1: pointer to a commonio_entry struct to compare
 * @p2: pointer to second commonio_entry struct to compare
 *
 * Returns 0 if the entries are the same.  Otherwise return -1
 * if the range in p1 is lower than that in p2, or (if the ranges are
 * equal) if the owning uid in p1 is lower than p2's.  Return 1 if p1's
 * range or owning uid is great than p2's.
 */
static int subordinate_range_cmp (const void *p1, const void *p2)
{
	struct subordinate_range *range1, *range2;

	if ((*(struct commonio_entry **) p1)->eptr == NULL)
		return 1;
	if ((*(struct commonio_entry **) p2)->eptr == NULL)
		return -1;

	range1 = ((struct subordinate_range *) (*(struct commonio_entry **) p1)->eptr);
	range2 = ((struct subordinate_range *) (*(struct commonio_entry **) p2)->eptr);

	if (range1->start < range2->start)
		return -1;
	else if (range1->start > range2->start)
		return 1;
	else if (range1->count < range2->count)
		return -1;
	else if (range1->count > range2->count)
		return 1;
	else
		return strcmp(range1->owner, range2->owner);
}

/*
 * find_free_range: find an unused consecutive sequence of ids to allocate
 *                  to a user.
 * @db: database to search
 * @min: the first uid in the range to find
 * @max: the highest uid to find
 * @count: the number of uids needed
 *
 * Return the lowest new uid, or ULONG_MAX on failure.
 */
static unsigned long find_free_range(struct commonio_db *db,
				     unsigned long min, unsigned long max,
				     unsigned long count)
{
	const struct subordinate_range *range;
	unsigned long low, high;

	/* When given invalid parameters fail */
	if ((count == 0) || (max < min))
		goto fail;

	/* Sort by range then by owner */
	commonio_sort (db, subordinate_range_cmp);
	commonio_rewind(db);

	low = min;
	while ((range = commonio_next(db)) != NULL) {
		unsigned long first = range->start;
		unsigned long last = first + range->count - 1;

		/* Find the top end of the hole before this range */
		high = first;

		/* Don't allocate IDs after max (included) */
		if (high > max + 1) {
			high = max + 1;
		}

		/* Is the hole before this range large enough? */
		if ((high > low) && ((high - low) >= count))
			return low;

		/* Compute the low end of the next hole */
		if (low < (last + 1))
			low = last + 1;
		if (low > max)
			goto fail;
	}

	/* Is the remaining unclaimed area large enough? */
	if (((max - low) + 1) >= count)
		return low;
fail:
	return ULONG_MAX;
}

/*
 * add_range: add a subuid range to an owning uid's list of authorized
 *            subuids.
 * @db: database to which to add
 * @owner: uid which owns the subuid
 * @start: the first uid in the owned range
 * @count: the number of uids in the range
 *
 * Return 1 if the range is already present or on success.  On error
 * return 0 and set errno appropriately.
 */
static int add_range(struct commonio_db *db,
	const char *owner, unsigned long start, unsigned long count)
{
	struct subordinate_range range;
	range.owner = owner;
	range.start = start;
	range.count = count;

	/* See if the range is already present */
	if (have_range(db, owner, start, count))
		return 1;

	/* Otherwise append the range */
	return commonio_append(db, &range);
}

/*
 * remove_range:  remove a range of subuids from an owning uid's list
 *                of authorized subuids.
 * @db: database to work on
 * @owner: owning uid whose range is being removed
 * @start: start of the range to be removed
 * @count: number of uids in the range.
 *
 * Returns 0 on failure, 1 on success.  Failure means that we needed to
 * create a new range to represent the new limits, and failed doing so.
 */
static int remove_range (struct commonio_db *db,
                         const char *owner,
                         unsigned long start, unsigned long count)
{
	struct commonio_entry *ent;
	unsigned long end;

	if (count == 0) {
		return 1;
	}

	end = start + count - 1;
	for (ent = db->head; NULL != ent; ent = ent->next) {
		struct subordinate_range *range = ent->eptr;
		unsigned long first;
		unsigned long last;

		/* Skip unparsed entries */
		if (NULL == range) {
			continue;
		}

		first = range->start;
		last = first + range->count - 1;

		/* Skip entries with a different owner */
		if (0 != strcmp (range->owner, owner)) {
			continue;
		}

		/* Skip entries outside of the range to remove */
		if ((end < first) || (start > last)) {
			continue;
		}

		if (start <= first) {
			if (end >= last) {
				/* to be removed: [start,      end]
				 * range:           [first, last] */
				/* entry completely contained in the
				 * range to remove */
				commonio_del_entry (db, ent);
			} else {
				/* to be removed: [start,  end]
				 * range:           [first, last] */
				/* Remove only the start of the entry */
				range->start = end + 1;
				range->count = (last - range->start) + 1;

				ent->changed = true;
				db->changed = true;
			}
		} else {
			if (end >= last) {
				/* to be removed:   [start,  end]
				 * range:         [first, last] */
				/* Remove only the end of the entry */
				range->count = start - range->start;

				ent->changed = true;
				db->changed = true;
			} else {
				/* to be removed:   [start, end]
				 * range:         [first,    last] */
				/* Remove the middle of the range
				 * This requires to create a new range */
				struct subordinate_range tail;
				tail.owner = range->owner;
				tail.start = end + 1;
				tail.count = (last - tail.start) + 1;

				if (commonio_append (db, &tail) == 0) {
					return 0;
				}

				range->count = start - range->start;

				ent->changed = true;
				db->changed = true;
			}
		}
	}

	return 1;
}

static struct commonio_db subordinate_uid_db = {
	"/etc/subuid",		/* filename */
	&subordinate_ops,	/* ops */
	NULL,			/* fp */
#ifdef WITH_SELINUX
	NULL,			/* scontext */
#endif
	0644,                   /* st_mode */
	0,                      /* st_uid */
	0,                      /* st_gid */
	NULL,			/* head */
	NULL,			/* tail */
	NULL,			/* cursor */
	false,			/* changed */
	false,			/* isopen */
	false,			/* locked */
	false			/* readonly */
};

int sub_uid_setdbname (const char *filename)
{
	return commonio_setname (&subordinate_uid_db, filename);
}

/*@observer@*/const char *sub_uid_dbname (void)
{
	return subordinate_uid_db.filename;
}

bool sub_uid_file_present (void)
{
	return commonio_present (&subordinate_uid_db);
}

int sub_uid_lock (void)
{
	return commonio_lock (&subordinate_uid_db);
}

int sub_uid_open (int mode)
{
	return commonio_open (&subordinate_uid_db, mode);
}

bool sub_uid_assigned(const char *owner)
{
	return range_exists (&subordinate_uid_db, owner);
}

bool have_sub_uids(const char *owner, uid_t start, unsigned long count)
{
	return have_range (&subordinate_uid_db, owner, start, count);
}

int sub_uid_add (const char *owner, uid_t start, unsigned long count)
{
	return add_range (&subordinate_uid_db, owner, start, count);
}

int sub_uid_remove (const char *owner, uid_t start, unsigned long count)
{
	return remove_range (&subordinate_uid_db, owner, start, count);
}

int sub_uid_close (void)
{
	return commonio_close (&subordinate_uid_db);
}

int sub_uid_unlock (void)
{
	return commonio_unlock (&subordinate_uid_db);
}

uid_t sub_uid_find_free_range(uid_t min, uid_t max, unsigned long count)
{
	unsigned long start;
	start = find_free_range (&subordinate_uid_db, min, max, count);
	return start == ULONG_MAX ? (uid_t) -1 : start;
}

static struct commonio_db subordinate_gid_db = {
	"/etc/subgid",		/* filename */
	&subordinate_ops,	/* ops */
	NULL,			/* fp */
#ifdef WITH_SELINUX
	NULL,			/* scontext */
#endif
	0644,                   /* st_mode */
	0,                      /* st_uid */
	0,                      /* st_gid */
	NULL,			/* head */
	NULL,			/* tail */
	NULL,			/* cursor */
	false,			/* changed */
	false,			/* isopen */
	false,			/* locked */
	false			/* readonly */
};

int sub_gid_setdbname (const char *filename)
{
	return commonio_setname (&subordinate_gid_db, filename);
}

/*@observer@*/const char *sub_gid_dbname (void)
{
	return subordinate_gid_db.filename;
}

bool sub_gid_file_present (void)
{
	return commonio_present (&subordinate_gid_db);
}

int sub_gid_lock (void)
{
	return commonio_lock (&subordinate_gid_db);
}

int sub_gid_open (int mode)
{
	return commonio_open (&subordinate_gid_db, mode);
}

bool have_sub_gids(const char *owner, gid_t start, unsigned long count)
{
	return have_range(&subordinate_gid_db, owner, start, count);
}

bool sub_gid_assigned(const char *owner)
{
	return range_exists (&subordinate_gid_db, owner);
}

int sub_gid_add (const char *owner, gid_t start, unsigned long count)
{
	return add_range (&subordinate_gid_db, owner, start, count);
}

int sub_gid_remove (const char *owner, gid_t start, unsigned long count)
{
	return remove_range (&subordinate_gid_db, owner, start, count);
}

int sub_gid_close (void)
{
	return commonio_close (&subordinate_gid_db);
}

int sub_gid_unlock (void)
{
	return commonio_unlock (&subordinate_gid_db);
}

gid_t sub_gid_find_free_range(gid_t min, gid_t max, unsigned long count)
{
	unsigned long start;
	start = find_free_range (&subordinate_gid_db, min, max, count);
	return start == ULONG_MAX ? (gid_t) -1 : start;
}
#else				/* !ENABLE_SUBIDS */
extern int errno;		/* warning: ANSI C forbids an empty source file */
#endif				/* !ENABLE_SUBIDS */

