/*
 * Copyright 1991 - 1993, Julianne Frances Haugh
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
 * 3. Neither the name of Julianne F. Haugh nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY JULIE HAUGH AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL JULIE HAUGH OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <config.h>

#include "rcsid.h"
RCSID(PKG_VER "$Id: logoutd.c,v 1.13 1999/06/07 16:40:45 marekm Exp $")

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <signal.h>
#include <utmp.h>
#include <fcntl.h>
#include "prototypes.h"
#include "defines.h"

#ifdef	SVR4
#include <libgen.h>
#endif

#ifdef	SVR4
#define	signal	sigset
#endif

static char	*Prog;

static char	*mesg_buf = "login time exceeded\r\n";  /* XXX warning: const */
static int	mesg_len = 21;
static int	mesg_size;

#ifndef HUP_MESG_FILE
#define HUP_MESG_FILE "/etc/logoutd.mesg"
#endif

/* local function prototypes */
static RETSIGTYPE reload_mesg P_((int));
static int check_login P_((const struct utmp *));
int main P_((int, char **));

/*
 * reload_mesg - reload the message that is output when killing a process
 */

static RETSIGTYPE
reload_mesg(int sig)
{
	int	fd;
	struct	stat	sb;

	signal (sig, reload_mesg);

	if (stat (HUP_MESG_FILE, &sb))
		return;

	if ((sb.st_mode & S_IFMT) != S_IFREG)
		return;

	if ((fd = open (HUP_MESG_FILE, O_RDONLY)) != -1) {
		if (sb.st_size + 1 > mesg_size) {
			if (mesg_buf && mesg_size)
				free (mesg_buf);

			mesg_len = sb.st_size;
			mesg_size = mesg_len + 1;
			if (! (mesg_buf = (char *) malloc (mesg_len + 1)))
				goto end;
		} else
			mesg_len = sb.st_size;

		if (read (fd, mesg_buf, mesg_len) != mesg_len) {
			mesg_len = 0;
			goto end;
		}
	} else
		return;

end:
	close (fd);
}

/*
 * check_login - check if user (struct utmp) allowed to stay logged in
 */
static int
check_login(const struct utmp *ut)
{
	char user[sizeof(ut->ut_user) + 1];
	time_t now;

	/*
	 * ut_user may not have the terminating NUL.
	 */
	strncpy(user, ut->ut_user, sizeof(ut->ut_user));
	user[sizeof(ut->ut_user)] = '\0';

	time(&now);

	/*
	 * Check if they are allowed to be logged in right now.
	 */
	if (!isttytime(user, ut->ut_line, now))
		return 0;
#if 0
	/*
	 * Check for how long they are allowed to stay logged in.
	 * XXX - not implemented yet.  Need to add a new field to
	 * /etc/porttime (login time limit in minutes, or no limit,
	 * based on username, tty, and time of login).
	 */
	if (now - ut->ut_time > get_time_limit(user, ut->ut_line, ut->ut_time))
		return 0;
#endif
	return 1;
}

/*
 * logoutd - logout daemon to enforce /etc/porttime file policy
 *
 *	logoutd is started at system boot time and enforces the login
 *	time and port restrictions specified in /etc/porttime.  The
 *	utmp file is periodically scanned and offending users are logged
 *	off from the system.
 */

int
main(int argc, char **argv)
{
	int i;
	int status;
	struct utmp *ut;
	char user[sizeof(ut->ut_user) + 1];  /* terminating NUL */
	char tty_name[sizeof(ut->ut_line) + 6];  /* /dev/ + NUL */
	int tty_fd;

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

#ifndef DEBUG
	for (i = 0;close (i) == 0;i++)
		;

#ifdef HAVE_SETPGRP
#ifdef SETPGRP_VOID
	setpgrp();  /* USG */
#else
	setpgrp(getpid(), getpid());
#endif
#else /* !HAVE_SETPGRP */
	setpgid(getpid(), getpid());  /* BSD || SUN || SUN4 */
#endif /* !HAVE_SETPGRP */

	reload_mesg (SIGHUP);

	/*
	 * Put this process in the background.
	 */

	if ((i = fork ()))
		exit (i < 0 ? 1:0);
#endif /* !DEBUG */

	/*
	 * Start syslogging everything
	 */

	Prog = Basename(argv[0]);

	openlog(Prog, LOG_PID|LOG_CONS|LOG_NOWAIT, LOG_AUTH);

	/*
	 * Scan the UTMP file once per minute looking for users that
	 * are not supposed to still be logged in.
	 */

	while (1) {
#ifndef DEBUG
		sleep(60);
#endif

		/* 
		 * Attempt to re-open the utmp file.  The file is only
		 * open while it is being used.
		 */

		setutent();

		/*
		 * Read all of the entries in the utmp file.  The entries
		 * for login sessions will be checked to see if the user
		 * is permitted to be signed on at this time.
		 */

		while ((ut = getutent())) {
#ifdef USER_PROCESS
			if (ut->ut_type != USER_PROCESS)
				continue;
#endif
			if (ut->ut_user[0] == '\0')
				continue;
			if (check_login(ut))
				continue;

			/*
			 * Put the rest of this in a child process.  This
			 * keeps the scan from waiting on other ports to die.
			 */

			if (fork() != 0)
				continue;

			if (strncmp(ut->ut_line, "/dev/", 5) != 0)
				strcpy(tty_name, "/dev/");
			else
				tty_name[0] = '\0';

			strcat(tty_name, ut->ut_line);
#ifndef O_NOCTTY
#define O_NOCTTY 0
#endif
			if ((tty_fd = open (tty_name,
					O_WRONLY|O_NDELAY|O_NOCTTY)) != -1) {
/* Suggested by Ivan Nejgebauar <ian@unsux.ns.ac.yu>: set OPOST
   before writing the message.  --marekm */
				TERMIO oldt, newt;

				GTTY(tty_fd, &oldt);
				newt = oldt;
#ifdef OPOST
				newt.c_oflag |= OPOST;
#else  /* XXX - I'm too young to know bsd sgtty, sorry :).  --marekm */
#endif
				STTY(tty_fd, &newt);
				write (tty_fd, mesg_buf, mesg_len);
				STTY(tty_fd, &oldt);
				close (tty_fd);
				sleep(10);
			}
#ifdef USER_PROCESS  /* USG_UTMP */
			if (ut->ut_pid > 1) {
				kill(- ut->ut_pid, SIGHUP);
				sleep(10);
				kill(- ut->ut_pid, SIGKILL);
			}
#else  /* BSD || SUN || SUN4 */
			/*
			 * vhangup() the line to kill try and kill
			 * whatever is out there using it.
			 */

			if ((tty_fd = open (tty_name, O_RDONLY|O_NDELAY)) == -1)
				continue;

			vhangup (tty_fd);
			close (tty_fd);
#endif  /* BSD || SUN || SUN4 */

#if 0
			SYSLOG((LOG_NOTICE,
				"logged off user `%.*s' on `%.*s'\n",
				(int) sizeof(ut->ut_user), ut->ut_user,
				(int) sizeof(ut->ut_line), ut->ut_line));
#else
			/* avoid gcc warnings about %.*s in syslog() */
			strncpy(user, ut->ut_line, sizeof(user) - 1);
			user[sizeof(user) - 1] = '\0';

			SYSLOG((LOG_NOTICE, "logged off user `%s' on `%s'\n",
				user, tty_name));
#endif

			/*
			 * This child has done all it can, drop dead.
			 */

			exit (0);
		}

		endutent();

		/*
		 * Reap any dead babies ...
		 */

		while (wait (&status) != -1)
			;
	}
	return 1;  /* not reached */
}
