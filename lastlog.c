/*
 * Copyright 1989, 1990, 1992, John F. Haugh II
 * All rights reserved.
 *
 * Permission is granted to copy and create derivative works for any
 * non-commercial purpose, provided this copyright notice is preserved
 * in all copies of source code, or included in human readable form
 * and conspicuously displayed on all copies of object code or
 * distribution media.
 *
 * Modified from faillog.c by Philip Street
 * Installation:  Maine National Guard
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

#define LASTFILE "/usr/adm/lastlog"

#include "config.h"
#include "lastlog.h"

#ifndef	lint
static	char	sccsid[] = "@(#)lastlog.c	3.2	10:30:33	18 Jul 1993";
#endif

FILE	*lastlogfile;	/* lastlog file stream */
off_t	user;		/* one single user, specified on command line */
int	days;		/* number of days to consider for print command */
time_t	seconds;	/* that number of days in seconds */

int	uflg;		/* set if user is a valid user id */
int	tflg;		/* print is restricted to most recent days */
struct	lastlog	lastlog; /* scratch structure to play with ... */
struct	stat	statbuf; /* fstat buffer for file size */

int freadcode;

extern	int	optind;
extern	char	*optarg;
extern	char	*asctime ();
extern	struct	passwd	*getpwuid ();
extern	struct	passwd	*getpwnam ();
extern	struct	passwd	*getpwent ();
extern	struct	tm	*localtime ();

#define	DAY	(24L*3600L)
#define	NOW	(time ((time_t *) 0))

main (argc, argv)
int	argc;
char	**argv;
{
	int	c;
	struct	passwd	*pwent;

	if ((lastlogfile = fopen (LASTFILE,"r")) == (FILE *) 0) {
		perror (LASTFILE);
		exit (1);
	}
	while ((c = getopt (argc, argv, "u:t:")) != EOF) {
		switch (c) {
			case 'u':
				pwent = getpwnam (optarg);
				if (! pwent) {
					fprintf (stderr, "Unknown User: %s\n", optarg);
					exit (1);
				}
				uflg++;
				user = pwent->pw_uid;
				break;
			case 't':
				days = atoi (optarg);
				seconds = days * DAY;
				tflg++;
				break;
		}
	}
	print ();
	fclose (lastlogfile);
	exit (0);
	/*NOTREACHED*/
}

print ()
{
	int	uid;
	off_t	offset;

	if (uflg) {
		offset = user * sizeof lastlog;
		fstat (fileno (lastlogfile), &statbuf);
		if (offset >= statbuf.st_size)
			return;

		fseek (lastlogfile, (off_t) user * sizeof lastlog, 0);
		if (fread ((char *) &lastlog, sizeof lastlog, 1, lastlogfile) == 1)
			print_one (user);
		else
			perror (LASTFILE);
	} else {
		for (uid = 0;
			fread ((char *) &lastlog, sizeof lastlog, 1, lastlogfile) == 1;
				uid++) {

			if (tflg && NOW - lastlog.ll_time > seconds)
				continue;

			print_one (uid);
		}
	}
}

print_one (uid)
int	uid;
{
	static	int	once;
	char	*cp;
	struct	tm	*tm;
	struct	passwd	*pwent;

	if (! once) {
		printf ("Username\t\tPort\tLatest\n");
		once++;
	}
	pwent = getpwuid (uid);
	tm = localtime (&lastlog.ll_time);
	cp = asctime (tm);
	cp[24] = '\0';

	if(lastlog.ll_time == (time_t) 0)
		cp = "**Never logged in**\0";

	if (pwent) {
		printf ("%-16s\t%s\t%s\n",pwent->pw_name,lastlog.ll_line,cp);
	}
}
