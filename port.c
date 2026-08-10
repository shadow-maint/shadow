/*
 * Copyright 1989, 1990, 1991, 1993, John F. Haugh II
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

#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#ifndef	BSD
#include <string.h>
#else
#include <strings.h>
#define	strchr	index
#define	strrchr	rindex
#endif
#include "port.h"

#ifndef	lint
static	char	_sccsid[] = "@(#)port.c	3.3	09:35:06	30 Apr 1993";
#endif

extern	int	errno;

static	FILE	*ports;

/*
 * portcmp - compare the name of a port to a /etc/porttime entry
 *
 *	portcmp works like strcmp, except that if the last character
 *	in a failing match is a '*', the match is considered to have
 *	passed.
 */

static int
portcmp (pattern, port)
char	*pattern;
char	*port;
{
	while (*pattern && *pattern == *port)
		pattern++, port++;

	return (*pattern == 0 && *port == 0) || *pattern == '*' ? 0:1;
}

/*
 * setportent - open /etc/porttime file or rewind
 *
 *	the /etc/porttime file is rewound if already open, or
 *	opened for reading.
 */

void
setportent ()
{
	if (ports)
		rewind (ports);
	else 
		ports = fopen (PORTS, "r");
}

/*
 * endportent - close the /etc/porttime file
 *
 *	the /etc/porttime file is closed and the ports variable set
 *	to NULL to indicate that the /etc/porttime file is no longer
 *	open.
 */

void
endportent ()
{
	if (ports)
		fclose (ports);

	ports = (FILE *) 0;
}

/*
 * getportent - read a single entry from /etc/porttime
 *
 *	the next line in /etc/porttime is converted to a (struct port)
 *	and a pointer to a static (struct port) is returned to the
 *	invoker.  NULL is returned on either EOF or error.  errno is
 *	set to EINVAL on error to distinguish the two conditions.
 */

struct port *
getportent ()
{
	static	struct	port	port;	/* static struct to point to         */
	static	char	buf[BUFSIZ];	/* some space for stuff              */
	static	char	*ttys[PORT_TTY+1]; /* some pointers to tty names     */
	static	char	*users[PORT_IDS+1]; /* some pointers to user ids     */
	static	struct	pt_time	times[PORT_TIMES+1]; /* time ranges          */
	char	*cp;			/* pointer into line                 */
	int	time;			/* scratch time of day               */
	int	i, j;
	int	saveerr = errno;	/* errno value on entry              */

	/*
	 * If the ports file is not open, open the file.  Do not rewind
	 * since we want to search from the beginning each time.
	 */

	if (! ports)
		setportent ();

	if (! ports) {
		errno = saveerr;
		return 0;
	}

	/*
	 * Common point for beginning a new line -
	 *
	 *	- read a line, and NUL terminate
	 *	- skip lines which begin with '#'
	 *	- parse off the tty names
	 *	- parse off a list of user names
	 *	- parse off a list of days and times
	 */

again:

	/*
	 * Get the next line and remove the last character, which
	 * is a '\n'.  Lines which begin with '#' are all ignored.
	 */

	if (fgets (buf, BUFSIZ, ports) == 0) {
		errno = saveerr;
		return 0;
	}
	if (buf[0] == '#')
		goto again;

	/*
	 * Get the name of the TTY device.  It is the first colon
	 * separated field, and is the name of the TTY with no
	 * leading "/dev".  The entry '*' is used to specify all
	 * TTY devices.
	 */

	buf[strlen (buf) - 1] = 0;

	port.pt_names = ttys;
	for (cp = buf, j = 0;j < PORT_TTY;j++) {
		port.pt_names[j] = cp;
		while (*cp && *cp != ':' && *cp != ',')
			cp++;

		if (! *cp)
			goto again;	/* line format error */

		if (*cp == ':')		/* end of tty name list */
			break;

		if (*cp == ',')		/* end of current tty name */
			*cp++ = '\0';
	}
	*cp++ = 0;
	port.pt_names[j + 1] = (char *) 0;

	/*
	 * Get the list of user names.  It is the second colon
	 * separated field, and is a comma separated list of user
	 * names.  The entry '*' is used to specify all usernames.
	 * The last entry in the list is a (char *) 0 pointer.
	 */

	if (*cp != ':') {
		port.pt_users = users;
		port.pt_users[0] = cp;

		for (j = 1;*cp != ':';cp++) {
			if (*cp == ',' && j < PORT_IDS) {
				*cp++ = 0;
				port.pt_users[j++] = cp;
			}
		}
		port.pt_users[j] = 0;
	} else
		port.pt_users = 0;

	if (*cp != ':')
		goto again;

	*cp++ = 0;

	/*
	 * Get the list of valid times.  The times field is the third
	 * colon separated field and is a list of days of the week and
	 * times during which this port may be used by this user.  The
	 * valid days are 'Su', 'Mo', 'Tu', 'We', 'Th', 'Fr', and 'Sa'.
	 *
	 * In addition, the value 'Al' represents all 7 days, and 'Wk'
	 * represents the 5 weekdays.
	 *
	 * Times are given as HHMM-HHMM.  The ending time may be before
	 * the starting time.  Days are presumed to wrap at 0000.
	 */

	if (*cp == '\0') {
		port.pt_times = 0;
		return &port;
	}

	port.pt_times = times;

	/*
	 * Get the next comma separated entry
	 */

	for (j = 0;*cp && j < PORT_TIMES;j++) {

		/*
		 * Start off with no days of the week
		 */

		port.pt_times[j].t_days = 0;

		/*
		 * Check each two letter sequence to see if it is
		 * one of the abbreviations for the days of the
		 * week or the other two values.
		 */

		for (i = 0;cp[i] && cp[i + 1] && isalpha (cp[i]);i += 2) {
			switch ((cp[i] << 8) | (cp[i + 1])) {
				case ('S' << 8) | 'u':
					port.pt_times[j].t_days |= 01;
					break;
				case ('M' << 8) | 'o':
					port.pt_times[j].t_days |= 02;
					break;
				case ('T' << 8) | 'u':
					port.pt_times[j].t_days |= 04;
					break;
				case ('W' << 8) | 'e':
					port.pt_times[j].t_days |= 010;
					break;
				case ('T' << 8) | 'h':
					port.pt_times[j].t_days |= 020;
					break;
				case ('F' << 8) | 'r':
					port.pt_times[j].t_days |= 040;
					break;
				case ('S' << 8) | 'a':
					port.pt_times[j].t_days |= 0100;
					break;
				case ('W' << 8) | 'k':
					port.pt_times[j].t_days |= 076;
					break;
				case ('A' << 8) | 'l':
					port.pt_times[j].t_days |= 0177;
					break;
				default:
					errno = EINVAL;
					return 0;
			}
		}

		/*
		 * The default is 'Al' if no days were seen.
		 */

		if (i == 0)
			port.pt_times[j].t_days = 0177;

		/*
		 * The start and end times are separated from each
		 * other by a '-'.  The times are four digit numbers
		 * representing the times of day.
		 */

		for (time = 0;cp[i] && isdigit (cp[i]);i++)
			time = time * 10 + cp[i] - '0';

		if (cp[i] != '-' || time > 2400 || time % 100 > 59)
			goto again;
		port.pt_times[j].t_start = time;
		cp = cp + i + 1;

		for (time = i = 0;cp[i] && isdigit (cp[i]);i++)
			time = time * 10 + cp[i] - '0';

		if ((cp[i] != ',' && cp[i]) || time > 2400 || time % 100 > 59)
			goto again;

		port.pt_times[j].t_end = time;
		cp = cp + i + 1;
	}

	/*
	 * The end of the list is indicated by a pair of -1's for the
	 * start and end times.
	 */

	port.pt_times[j].t_start = port.pt_times[j].t_end = -1;

	return &port;
}

/*
 * getttyuser - get ports information for user and tty
 *
 *	getttyuser() searches the ports file for an entry with a TTY
 *	and user field both of which match the supplied TTY and
 *	user name.  The file is searched from the beginning, so the
 *	entries are treated as an ordered list.
 */

struct port *
getttyuser (tty, user)
char	*tty;
char	*user;
{
	int	i, j;
	struct	port	*port;

	setportent ();

	while (port = getportent ()) {
		if (port->pt_names == 0 || port->pt_users == 0)
			continue;

		for (i = 0;port->pt_names[i];i++)
			if (portcmp (port->pt_names[i], tty) == 0)
				break;

		if (port->pt_names[i] == 0)
			continue;

		for (j = 0;port->pt_users[j];j++)
			if (strcmp (user, port->pt_users[j]) == 0 ||
					strcmp (port->pt_users[j], "*") == 0)
				break;

		if (port->pt_users[j] != 0)
			break;
	}
	endportent ();
	return port;
}

/*
 * isttytime - tell if a given user may login at a particular time
 *
 *	isttytime searches the ports file for an entry which matches
 *	the user name and TTY given.
 */

int
isttytime (id, port, clock)
char	*id;
char	*port;
long	clock;
{
	int	i;
	int	time;
	struct	port	*pp;
	struct	tm	*tm,
			*localtime();

	/*
	 * Try to find a matching entry for this user.  Default to
	 * letting the user in - there are pleny of ways to have an
	 * entry to match all users.
	 */

	if (! (pp = getttyuser (port, id)))
		return 1;

	/*
	 * The entry is there, but has not time entries - don't
	 * ever let them login.
	 */

	if (pp->pt_times == 0)
		return 0;

	/*
	 * The current time is converted to HHMM format for
	 * comparision against the time values in the TTY entry.
	 */

	tm = localtime (&clock);
	time = tm->tm_hour * 100 + tm->tm_min;

	/*
	 * Each time entry is compared against the current
	 * time.  For entries with the start after the end time,
	 * the comparision is made so that the time is between
	 * midnight and either the start or end time.
	 */

	for (i = 0;pp->pt_times[i].t_start != -1;i++) {
		if (! (pp->pt_times[i].t_days & PORT_DAY(tm->tm_wday)))
			continue;

		if (pp->pt_times[i].t_start <= pp->pt_times[i].t_end) {
			if (time >= pp->pt_times[i].t_start &&
					time <= pp->pt_times[i].t_end)
				return 1;
		} else {
			if (time >= pp->pt_times[i].t_start ||
					time <= pp->pt_times[i].t_end)
				return 1;
		}
	}

	/*
	 * No matching time entry was found, user shouldn't
	 * be let in right now.
	 */

	return 0;
}
