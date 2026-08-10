/*
 * Copyright 1989, 1990, 1991, 1992, 1993, John F. Haugh II
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
 */

#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <stdio.h>
#ifndef	BSD
#include <string.h>
#include <memory.h>
#else
#include <strings.h>
#define	strchr	index
#define	strrchr	rindex
#endif
#ifdef	UNISTD_H
#include <unistd.h>
#endif
#include "faillog.h"
#include "config.h"

#include <utmp.h>

#ifndef	lint
static	char	_sccsid[] = "@(#)failure.c	3.3	08:01:05	22 Apr 1993";
#endif

#define	DAY	(24L*3600L)
#define	YEAR	(365L*DAY)
#define	NOW	(time ((time_t *) 0))

extern	struct	tm	*localtime ();
extern	char	*asctime ();
extern	void	failprint ();
extern	char	*getdef_str();

/*
 * failure - make failure entry
 *
 *	failure() creates a new (struct faillog) entry or updates an
 *	existing one with the current failed login information.
 */

void
failure (uid, tty, faillog)
int	uid;
char	*tty;
struct	faillog	*faillog;
{
	int	fd;

	/*
	 * Do do anything if failure logging isn't set up.
	 */

	if ((fd = open (FAILFILE, O_RDWR)) < 0)
		return;

	/*
	 * The file is indexed by uid value meaning that shared UID's
	 * share failure log records.  That's OK since they really
	 * share just about everything else ...
	 */

	lseek (fd, (off_t) (sizeof *faillog) * uid, 0);
	if (read (fd, (char *) faillog, sizeof *faillog)
			!= sizeof *faillog)
#ifndef	BSD
		memset ((void *) faillog, 0, sizeof *faillog);
#else
		bzero ((char *) faillog, sizeof *faillog);
#endif

	/*
	 * Update the record.  We increment the failure count to log the
	 * latest failure.  The only concern here is overflow, and we'll
	 * check for that.  The line name and time of day are both
	 * updated as well.
	 */

	if (faillog->fail_cnt + 1 > 0)
		faillog->fail_cnt++;

	strncpy (faillog->fail_line, tty, sizeof faillog->fail_line);
	faillog->fail_time = time ((time_t *) 0);

	/*
	 * Seek back to the correct position in the file and write the
	 * record out.  Ideally we should lock the file in case the same
	 * account is being logged simultaneously.  But the risk doesn't
	 * seem that great.
	 */

	lseek (fd, (off_t) (sizeof *faillog) * uid, 0);
	write (fd, (char *) faillog, sizeof *faillog);
	close (fd);
}

/*
 * failcheck - check for failures > allowable
 *
 *	failcheck() is called AFTER the password has been validated.  If the
 *	account has been "attacked" with too many login failures, failcheck()
 *	returns FALSE to indicate that the login should be denied even though
 *	the password is valid.
 */

int
failcheck (uid, faillog, failed)
int	uid;
struct	faillog	*faillog;
int	failed;
{
	int	fd;
	int	okay = 1;
	struct	faillog	fail;

	/*
	 * Suppress the check if the log file isn't there.
	 */

	if ((fd = open (FAILFILE, O_RDWR)) < 0)
		return (1);

	/*
	 * Get the record from the file and determine if the user has
	 * exceeded the failure limit.  If "max" is zero, any number
	 * of failures are permitted.  Only when "max" is non-zero and
	 * "cnt" is greater than or equal to "max" is the account
	 * considered to be locked.
	 */

	lseek (fd, (off_t) (sizeof *faillog) * uid, 0);
	if (read (fd, (char *) faillog, sizeof *faillog) == sizeof *faillog) {
		if (faillog->fail_max != 0
				&& faillog->fail_cnt >= faillog->fail_max)
			okay = 0;
	}

	/*
	 * The record is updated if this is not a failure.  The count will
	 * be reset to zero, but the rest of the information will be left
	 * in the record in case someone wants to see where the failed
	 * login originated.
	 */

	if (!failed && okay) {
		fail = *faillog;
		fail.fail_cnt = 0;

		lseek (fd, (off_t) sizeof fail * uid, 0);
		write (fd, (char *) &fail, sizeof fail);
	}
	close (fd);

	return (okay);
}

/*
 * failprint - print line of failure information
 *
 *	failprint takes a (struct faillog) entry and formats it into a
 *	message which is displayed at login time.
 */

void
failprint (fail)
struct	faillog	*fail;
{
	struct	tm	*tp;
#ifdef	SVR4
	char	lasttimeb[32];
	char	*lasttime = lasttimeb;
#else
	char	*lasttime;
#endif

	if (fail->fail_cnt == 0)
		return;

	tp = localtime (&(fail->fail_time));

#if __STDC__
	/*
	 * Only print as much date and time info as it needed to
	 * know when the failure was.
	 */

	if (NOW - fail->fail_time >= YEAR)
	    strftime(lasttime, sizeof lasttime, NULL, tp);
	else if (NOW - fail->fail_time >= DAY)
	    strftime(lasttime, sizeof lasttime, "%A %T", tp);
	else
	    strftime(lasttime, sizeof lasttime, "%T", tp);
#else

	/*
	 * Do the same thing, but don't use strftime since it
	 * probably doesn't exist on this system
	 */

	lasttime = asctime (tp);
	lasttime[24] = '\0';

	if (NOW - fail->fail_time < YEAR)
		lasttime[19] = '\0';
	if (NOW - fail->fail_time < DAY)
		lasttime = lasttime + 11;

	if (*lasttime == ' ')
		lasttime++;
#endif	/* __STDC__ */
	printf ("%d %s since last login.  Last was %s on %s.\n",
		fail->fail_cnt, fail->fail_cnt > 1 ? "failures":"failure",
		lasttime, fail->fail_line);
}

/*
 * failtmp - update the cummulative failure log
 *
 *	failtmp updates the (struct utmp) formatted failure log which
 *	maintains a record of all login failures.
 */

void
failtmp (failent)
struct	utmp	*failent;
{
	int	fd;
	char	*ftmp;

	/*
	 * Get the name of the failure file.  If no file has been defined
	 * in login.defs, don't do this.
	 */

	if ((ftmp = getdef_str ("FTMP_FILE")) == 0)
		return;

	/*
	 * Open the file for append.  It must already exist for this
	 * feature to be used.
	 */

	if ((fd = open (ftmp, O_WRONLY|O_APPEND)) == -1)
		return;

	/*
	 * Output the new failure record and close the log file.
	 */

	write (fd, (char *) failent, sizeof *failent);
	close (fd);
}
