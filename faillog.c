/*
 * Copyright 1989, 1990, 1992, John F. Haugh II
 * All rights reserved.
 *
 * Permission is granted to copy and create derivative works for any
 * non-commercial purpose, provided this copyright notice is preserved
 * in all copies of source code, or included in human readable form
 * and conspicuously displayed on all copies of object code or
 * distribution media.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include "pwd.h"
#include <time.h>
#ifndef	BSD
#include <string.h>
#include <memory.h>
#else
#include <strings.h>
#define	strchr	index
#define	strrchr	rindex
#endif
#ifdef	STDLIB_H
#include <stdlib.h>
#endif
#ifdef	UNISTD_H
#include <unistd.h>
#endif
#include "config.h"
#include "faillog.h"

#ifndef	lint
static	char	_sccsid[] = "@(#)faillog.c	3.3	20:36:23	07 Mar 1992";
#endif

FILE	*fail;		/* failure file stream */
uid_t	user;		/* one single user, specified on command line */
int	days;		/* number of days to consider for print command */
time_t	seconds;	/* that number of days in seconds */
int	max;		/* maximum failure count for fail_max */

int	aflg;		/* set if all users are to be printed always */
int	uflg;		/* set if user is a valid user id */
int	tflg;		/* print is restricted to most recent days */
struct	stat	statbuf; /* fstat buffer for file size */

#if !defined(UNISTD_H) && !defined(STDLIB_H)
extern	int	optind;
extern	char	*optarg;
extern	char	*asctime ();
extern	struct	passwd	*getpwuid ();
extern	struct	passwd	*getpwnam ();
extern	struct	passwd	*getpwent ();
extern	struct	tm	*localtime ();
#endif

#if	__STDC__
void	print(void);
void	print_one(struct faillog *faillog, uid_t uid);
void	reset(void);
int	reset_one(uid_t uid);
void	setmax(void);
void	setmax_one(uid_t uid);
#else
void	print();
void	print_one();
void	reset();
int	reset_one();
void	setmax();
void	setmax_one();
#endif /* __STDC__ */

#define	DAY	(24L*3600L)
#define	NOW	(time ((time_t *) 0))

void
main (argc, argv)
int	argc;
char	**argv;
{
	char	*mode;
	int	c;
	struct	passwd	*pwent;

	if (getuid () == 0)	/* only root can update anything */
		mode = "r+";
	else			/* all others can only look */
		mode = "r";

	if ((fail = fopen (FAILFILE, mode)) == (FILE *) 0) {
		perror (FAILFILE);
		exit (1);
	}
	while ((c = getopt (argc, argv, "am:pru:t:")) != EOF) {
		switch (c) {
			case 'a':
				aflg++;
				uflg = 0;
				break;
			case 'm':
				max = atoi (optarg);
				setmax ();
				break;
			case 'p':
				print ();
				break;
			case 'r':
				reset ();
				break;
			case 'u':
				pwent = getpwnam (optarg);
				if (! pwent) {
					fprintf (stderr, "Unknown User: %s\n", optarg);
					exit (1);
				}
				uflg++;
				aflg = 0;
				user = pwent->pw_uid;
				break;
			case 't':
				days = atoi (optarg);
				seconds = days * DAY;
				tflg++;
				break;
		}
	}
	fclose (fail);
	exit (0);
	/*NOTREACHED*/
}

void
print ()
{
	uid_t	uid;
	off_t	offset;
	struct	faillog	faillog;

	if (uflg) {
		offset = user * sizeof faillog;
		fstat (fileno (fail), &statbuf);
		if (offset >= statbuf.st_size)
			return;

		fseek (fail, (off_t) user * sizeof faillog, 0);
		if (fread ((char *) &faillog, sizeof faillog, 1, fail) == 1)
			print_one (&faillog, user);
		else
			perror (FAILFILE);
	} else {
		for (uid = 0;
			fread ((char *) &faillog, sizeof faillog, 1, fail) == 1;
				uid++) {

			if (aflg == 0 && faillog.fail_cnt == 0)
				continue;

			if (aflg == 0 && tflg &&
					NOW - faillog.fail_time > seconds)
				continue;

			if (aflg && faillog.fail_time == 0)
				continue;

			print_one (&faillog, uid);
		}
	}
}

void
print_one (faillog, uid)
struct	faillog	*faillog;
uid_t	uid;
{
	static	int	once;
	char	*cp;
	struct	tm	*tm;
	struct	passwd	*pwent;

	if (! once) {
		printf ("Username        Failures    Maximum     Latest\n");
		once++;
	}
	pwent = getpwuid (uid);
	tm = localtime (&faillog->fail_time);
	cp = asctime (tm);
	cp[24] = '\0';

	if (pwent) {
		printf ("%-16s    %4d       %4d",
			pwent->pw_name, faillog->fail_cnt, faillog->fail_max);
		if (faillog->fail_time)
			printf ("     %s on %s\n", cp, faillog->fail_line);
		else
			putchar ('\n');
	}
}

void
reset ()
{
	int	uid = 0;

	if (uflg)
		reset_one (user);
	else
		for (uid = 0;reset_one (uid);uid++)
			;
}

int
reset_one (uid)
uid_t	uid;
{
	off_t	offset;
	struct	faillog	faillog;

	offset = uid * sizeof faillog;
	fstat (fileno (fail), &statbuf);
	if (offset >= statbuf.st_size)
		return (0);

	if (fseek (fail, offset, 0) != 0) {
		perror (FAILFILE);
		return (0);
	}
	if (fread ((char *) &faillog, sizeof faillog, 1, fail) != 1) {
		if (! feof (fail))
			perror (FAILFILE);

		return (0);
	}
	if (faillog.fail_cnt == 0)
		return (1);	/* don't fill in no holes ... */

	faillog.fail_cnt = 0;

	if (fseek (fail, offset, 0) == 0
		&& fwrite ((char *) &faillog, sizeof faillog, 1, fail) == 1) {
		fflush (fail);
		return (1);
	} else {
		perror (FAILFILE);
	}
	return (0);
}

void
setmax ()
{
	struct	passwd	*pwent;

	if (uflg) {
		setmax_one (user);
	} else {
		setpwent ();
		while (pwent = getpwent ())
			setmax_one (pwent->pw_uid);
	}
}

void
setmax_one (uid)
uid_t	uid;
{
	off_t	offset;
	struct	faillog	faillog;

	offset = uid * sizeof faillog;

	if (fseek (fail, offset, 0) != 0) {
		perror (FAILFILE);
		return;
	}
	if (fread ((char *) &faillog, sizeof faillog, 1, fail) != 1) {
		if (! feof (fail))
			perror (FAILFILE);
	} else {
#ifndef	BSD
		memset ((char *) &faillog, 0, sizeof faillog);
#else
		bzero ((char *) &faillog, sizeof faillog);
#endif
	}
	faillog.fail_max = max;

	if (fseek (fail, offset, 0) == 0
		&& fwrite ((char *) &faillog, sizeof faillog, 1, fail) == 1)
		fflush (fail);
	else
		perror (FAILFILE);
}
