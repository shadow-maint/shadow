/*
 * SPDX-FileCopyrightText: 1989 - 1994, Julianne Frances Haugh
 * SPDX-FileCopyrightText: 1996 - 1999, Marek Michałkiewicz
 * SPDX-FileCopyrightText: 2003 - 2005, Tomasz Kłoczko
 * SPDX-FileCopyrightText: 2007 - 2008, Nicolas François
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <config.h>

#ifdef RLOGIN

#ident "$Id$"

#include "prototypes.h"
#include "defines.h"
#include <stdio.h>
#include <pwd.h>
#include <netdb.h>

#include "atoi/getlong.h"


static struct {
	int spd_name;
	int spd_baud;
} speed_table[] =
{
	{ B50, 50},
	{ B75, 75},
	{ B110, 110},
	{ B134, 134},
	{ B150, 150},
	{ B200, 200},
	{ B300, 300},
	{ B600, 600},
	{ B1200, 1200},
	{ B1800, 1800},
	{ B2400, 2400},
	{ B4800, 4800},
	{ B9600, 9600},
	{ B19200, 19200},
	{ B38400, 38400},
	{ -1, -1}
};

static void get_remote_string (char *buf, size_t size)
{
	for (;;) {
		if (read (0, buf, 1) != 1) {
			exit (EXIT_FAILURE);
		}
		if ('\0' == *buf) {
			return;
		}
		--size;
		if (size > 0) {
			++buf;
		}
	}
 /*NOTREACHED*/}

int
do_rlogin (const char *remote_host, char *name, size_t namelen, char *term,
           size_t termlen)
{
	struct passwd *pwd;
	char remote_name[32];
	char *cp;
	unsigned long remote_speed = 9600;
	int speed_name = B9600;
	int i;
	TERMIO termio;

	get_remote_string (remote_name, sizeof remote_name);
	get_remote_string (name, namelen);
	get_remote_string (term, termlen);

	cp = strchr (term, '/');
	if (NULL != cp) {
		*cp = '\0';
		cp++;

		if (getul(cp, &remote_speed) == -1) {
			remote_speed = 9600;
		}
	}
	for (i = 0;
	     (   (speed_table[i].spd_baud != remote_speed)
	      && (speed_table[i].spd_name != -1));
	     i++);

	if (-1 != speed_table[i].spd_name) {
		speed_name = speed_table[i].spd_name;
	}

	/*
	 * Put the terminal in cooked mode with echo turned on.
	 */

	GTTY (0, &termio);
	termio.c_iflag |= ICRNL | IXON;
	termio.c_oflag |= OPOST | ONLCR;
	termio.c_lflag |= ICANON | ECHO | ECHOE;
#ifdef CBAUD
	termio.c_cflag = (termio.c_cflag & ~CBAUD) | speed_name;
#else
	termio.c_cflag = (termio.c_cflag) | speed_name;
#endif
	STTY (0, &termio);

	pwd = getpwnam (name); /* local, no need for xgetpwnam */
	if (NULL == pwd) {
		return 0;
	}

	/*
	 * ruserok() returns 0 for success on modern systems, and 1 on
	 * older ones.  If you are having trouble with people logging
	 * in without giving a required password, THIS is the culprit -
	 * go fix the #define in config.h.
	 */

#ifndef	RUSEROK
	return 0;
#else
	return ruserok (remote_host, pwd->pw_uid == 0,
			remote_name, name) == RUSEROK;
#endif
}
#endif				/* RLOGIN */

