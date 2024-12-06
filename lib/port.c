/*
 * SPDX-FileCopyrightText: 1989 - 1994, Julianne Frances Haugh
 * SPDX-FileCopyrightText: 1996 - 1997, Marek Michałkiewicz
 * SPDX-FileCopyrightText: 2005       , Tomasz Kłoczko
 * SPDX-FileCopyrightText: 2008       , Nicolas François
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <config.h>

#ident "$Id$"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "defines.h"
#include "port.h"
#include "prototypes.h"
#include "string/strcmp/streq.h"
#include "string/strcmp/strprefix.h"
#include "string/strtok/stpsep.h"
#include "string/strtok/strsep2ls.h"


static FILE *ports;

/*
 * portcmp - compare the name of a port to a /etc/porttime entry
 *
 *	portcmp works like strcmp, except that if the last character
 *	in a failing match is a '*', the match is considered to have
 *	passed.  The "*" match is suppressed whenever the port is "SU",
 *	which is the token the "su" command uses to validate access.
 *	A match returns 0, failure returns non-zero.
 */

static int portcmp (const char *pattern, const char *port)
{
	const char *orig = port;

	while (!streq(pattern, "") && (*pattern == *port)) {
		pattern++;
		port++;
	}

	if (streq(pattern, "") && streq(port, "")) {
		return 0;
	}
	if (streq(orig, "SU"))
		return 1;

	return !strprefix(pattern, "*");
}

/*
 * setportent - open /etc/porttime file or rewind
 *
 *	the /etc/porttime file is rewound if already open, or
 *	opened for reading.
 */

static void setportent (void)
{
	if (NULL != ports) {
		rewind (ports);
	} else {
		ports = fopen (PORTS, "r");
	}
}

/*
 * endportent - close the /etc/porttime file
 *
 *	the /etc/porttime file is closed and the ports variable set
 *	to NULL to indicate that the /etc/porttime file is no longer
 *	open.
 */

static void endportent (void)
{
	if (NULL != ports) {
		(void) fclose (ports);
	}

	ports = NULL;
}

/*
 * getportent - read a single entry from /etc/porttime
 *
 *	the next line in /etc/porttime is converted to a (struct port)
 *	and a pointer to a static (struct port) is returned to the
 *	invoker.  NULL is returned on either EOF or error.  errno is
 *	set to EINVAL on error to distinguish the two conditions.
 */

static struct port *
getportent(void)
{
	int   dtime;
	int   i, j;
	int   saveerr;
	char  *cp;
	char  *fields[3];

	static char            buf[BUFSIZ];
	static char            *ttys[PORT_TTY + 1];
	static char            *users[PORT_IDS + 1];
	static struct port     port;
	static struct pt_time  ptimes[PORT_TIMES + 1];

	saveerr = errno;

	/*
	 * If the ports file is not open, open the file.  Do not rewind
	 * since we want to search from the beginning each time.
	 */

	if (NULL == ports) {
		setportent ();
	}

	if (NULL == ports) {
		errno = saveerr;
		return NULL;
	}

	/*
	 * Common point for beginning a new line -
	 *
	 *      - read a line, and NUL terminate
	 *      - skip lines which begin with '#'
	 *      - parse off the tty names
	 *      - parse off a list of user names
	 *      - parse off a list of days and times
	 */

next:
	if (fgets(buf, sizeof(buf), ports) == NULL) {
		errno = saveerr;
		return NULL;
	}
	if (strprefix(buf, "#"))
		goto next;

	stpsep(buf, "\n");

	if (STRSEP2ARR(buf, ":", fields) == -1)
		goto next;

	/*
	 * Get the name of the TTY device.  It is the first colon
	 * separated field, and is the name of the TTY with no
	 * leading "/dev".  The entry '*' is used to specify all
	 * TTY devices.
	 */
	port.pt_names = ttys;
	if (STRSEP2LS(fields[0], ",", ttys) == -1)
		goto next;

	/*
	 * Get the list of user names.  It is the second colon
	 * separated field, and is a comma separated list of user
	 * names.  The entry '*' is used to specify all usernames.
	 * The last entry in the list is a NULL pointer.
	 */
	port.pt_users = users;
	if (STRSEP2LS(fields[1], ",", users) == -1)
		goto next;

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

	cp = fields[2];

	if (streq(cp, "")) {
		port.pt_times = NULL;
		return &port;
	}

	port.pt_times = ptimes;

	/*
	 * Get the next comma separated entry
	 */

	for (j = 0; !streq(cp, "") && (j < PORT_TIMES); j++) {

		/*
		 * Start off with no days of the week
		 */

		port.pt_times[j].t_days = 0;

		/*
		 * Check each two letter sequence to see if it is
		 * one of the abbreviations for the days of the
		 * week or the other two values.
		 */

		for (i = 0; isalpha(cp[i]) && ('\0' != cp[i + 1]); i += 2) {
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
				return NULL;
			}
		}

		/*
		 * The default is 'Al' if no days were seen.
		 */

		if (0 == i) {
			port.pt_times[j].t_days = 0177;
		}

		/*
		 * The start and end times are separated from each
		 * other by a '-'.  The times are four digit numbers
		 * representing the times of day.
		 */

		for (dtime = 0; isdigit (cp[i]); i++) {
			dtime = dtime * 10 + cp[i] - '0';
		}

		if (('-' != cp[i]) || (dtime > 2400) || ((dtime % 100) > 59)) {
			goto next;
		}
		port.pt_times[j].t_start = dtime;
		cp = cp + i + 1;

		for (dtime = 0, i = 0; isdigit (cp[i]); i++) {
			dtime = dtime * 10 + cp[i] - '0';
		}

		if (   ((',' != cp[i]) && ('\0' != cp[i]))
		    || (dtime > 2400)
		    || ((dtime % 100) > 59)) {
			goto next;
		}

		port.pt_times[j].t_end = dtime;
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

static struct port *
getttyuser(const char *tty, const char *user)
{
	struct port  *port;

	setportent();

	while ((port = getportent()) != NULL) {
		char  **ptn;
		char  **ptu;

		for (ptn = port->pt_names; *ptn != NULL; ptn++) {
			if (portcmp(*ptn, tty) == 0)
				break;
		}
		if (*ptn == NULL)
			continue;

		for (ptu = port->pt_users; *ptu != NULL; ptu++) {
			if (streq(*ptu, user))
				goto end;
			if (streq(*ptu, "*"))
				goto end;
		}
	}
end:
	endportent();
	return port;
}

/*
 * isttytime - tell if a given user may login at a particular time
 *
 *	isttytime searches the ports file for an entry which matches
 *	the user name and TTY given.
 */

bool isttytime (const char *id, const char *port, time_t when)
{
	int i;
	int dtime;
	struct port *pp;
	struct tm *tm;

	/*
	 * Try to find a matching entry for this user.  Default to
	 * letting the user in - there are plenty of ways to have an
	 * entry to match all users.
	 */

	pp = getttyuser (port, id);
	if (NULL == pp) {
		return true;
	}

	/*
	 * The entry is there, but has no time entries - don't
	 * ever let them login.
	 */

	if (NULL == pp->pt_times) {
		return false;
	}

	/*
	 * The current time is converted to HHMM format for
	 * comparison against the time values in the TTY entry.
	 */

	tm = localtime (&when);
	dtime = tm->tm_hour * 100 + tm->tm_min;

	/*
	 * Each time entry is compared against the current
	 * time.  For entries with the start after the end time,
	 * the comparison is made so that the time is between
	 * midnight and either the start or end time.
	 */

	for (i = 0; pp->pt_times[i].t_start != -1; i++) {
		if (!(pp->pt_times[i].t_days & PORT_DAY (tm->tm_wday))) {
			continue;
		}

		if (pp->pt_times[i].t_start <= pp->pt_times[i].t_end) {
			if (   (dtime >= pp->pt_times[i].t_start)
			    && (dtime <= pp->pt_times[i].t_end)) {
				return true;
			}
		} else {
			if (   (dtime >= pp->pt_times[i].t_start)
			    || (dtime <= pp->pt_times[i].t_end)) {
				return true;
			}
		}
	}

	/*
	 * No matching time entry was found, user shouldn't
	 * be let in right now.
	 */

	return false;
}

