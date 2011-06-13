/*
 * Copyright (c) 1989 - 1994, Julianne Frances Haugh
 * Copyright (c) 1996 - 2000, Marek Michałkiewicz
 * Copyright (c) 2000 - 2006, Tomasz Kłoczko
 * Copyright (c) 2007 - 2011, Nicolas François
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
 * 3. The name of the copyright holders or contributors may not be used to
 *    endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* Some parts substantially derived from an ancestor of:
   su for GNU.  Run a shell with substitute user and group IDs.

   Copyright (C) 1992-2003 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */


#include <config.h>

#ident "$Id$"

#include <getopt.h>
#include <grp.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include "prototypes.h"
#include "defines.h"
#include "pwauth.h"
#include "getdef.h"
#ifdef USE_PAM
#include "pam_defs.h"
#endif				/* USE_PAM */
/*@-exitarg@*/
#include "exitcodes.h"

/*
 * Assorted #defines to control su's behavior
 */
/*
 * Global variables
 */
const char *Prog;

/* not needed by sulog.c anymore */
static char name[BUFSIZ];
static char oldname[BUFSIZ];

/* If nonzero, change some environment vars to indicate the user su'd to. */
static bool change_environment;

#ifdef USE_PAM
static pam_handle_t *pamh = NULL;
static int caught = 0;
/* PID of the child, in case it needs to be killed */
static pid_t pid_child = 0;
#endif

extern struct passwd pwent;

/*
 * External identifiers
 */

extern char **newenvp;
extern char **environ;
extern size_t newenvc;

/* local function prototypes */

static void execve_shell (const char *shellstr,
                          char *args[],
                          char *const envp[]);
#ifdef USE_PAM
static RETSIGTYPE kill_child (int unused(s));
#else				/* !USE_PAM */
static RETSIGTYPE die (int);
static bool iswheel (const char *);
#endif				/* !USE_PAM */

#ifndef USE_PAM
/*
 * die - set or reset termio modes.
 *
 *	die() is called before processing begins. signal() is then called
 *	with die() as the signal handler. If signal later calls die() with a
 *	signal number, the terminal modes are then reset.
 */
static RETSIGTYPE die (int killed)
{
	static TERMIO sgtty;

	if (killed != 0) {
		STTY (0, &sgtty);
	} else {
		GTTY (0, &sgtty);
	}

	if (killed != 0) {
		closelog ();
		exit (128+killed);
	}
}

static bool iswheel (const char *username)
{
	struct group *grp;

	grp = getgrnam ("wheel"); /* !USE_PAM, no need for xgetgrnam */
	if (   (NULL ==grp)
	    || (NULL == grp->gr_mem)) {
		return false;
	}
	return is_on_list (grp->gr_mem, username);
}
#else				/* USE_PAM */
static RETSIGTYPE kill_child (int unused(s))
{
	if (0 != pid_child) {
		(void) kill (pid_child, SIGKILL);
		(void) fputs (_(" ...killed.\n"), stderr);
	} else {
		(void) fputs (_(" ...waiting for child to terminate.\n"),
		              stderr);
	}
	exit (255);
}
#endif				/* USE_PAM */

/* borrowed from GNU sh-utils' "su.c" */
static bool restricted_shell (const char *shellstr)
{
	char *line;

	setusershell ();
	while ((line = getusershell ()) != NULL) {
		if (('#' != *line) && (strcmp (line, shellstr) == 0)) {
			endusershell ();
			return false;
		}
	}
	endusershell ();
	return true;
}

static void su_failure (const char *tty)
{
	sulog (tty, false, oldname, name);	/* log failed attempt */
#ifdef USE_SYSLOG
	if (getdef_bool ("SYSLOG_SU_ENAB")) {
		SYSLOG (((0 != pwent.pw_uid) ? LOG_INFO : LOG_NOTICE,
		         "- %s %s:%s", tty,
		         ('\0' != oldname[0]) ? oldname : "???",
		         ('\0' != name[0]) ? name : "???"));
	}
	closelog ();
#endif
	exit (1);
}

/*
 * execve_shell - Execute a shell with execve, or interpret it with
 * /bin/sh
 */
static void execve_shell (const char *shellstr,
                          char *args[],
                          char *const envp[])
{
	int err;
	(void) execve (shellstr, (char **) args, envp);
	err = errno;

	if (access (shellstr, R_OK|X_OK) == 0) {
		/*
		 * Assume this is a shell script (with no shebang).
		 * Interpret it with /bin/sh
		 */
		size_t n_args = 0;
		char **targs;
		while (NULL != args[n_args]) {
			n_args++;
		}
		targs = (char **) xmalloc ((n_args + 3) * sizeof (args[0]));
		targs[0] = "sh";
		targs[1] = "-";
		targs[2] = xstrdup (shellstr);
		targs[n_args+2] = NULL;
		while (1 != n_args) {
			targs[n_args+1] = args[n_args - 1];
			n_args--;
		}

		(void) execve (SHELL, targs, envp);
	} else {
		errno = err;
	}
}

#ifdef USE_PAM
/* Signal handler for parent process later */
static void catch_signals (int sig)
{
	caught = sig;
}

/* This I ripped out of su.c from sh-utils after the Mandrake pam patch
 * have been applied.  Some work was needed to get it integrated into
 * su.c from shadow.
 */
static void run_shell (const char *shellstr, char *args[], bool doshell,
                       char *const envp[])
{
	pid_t child;
	sigset_t ourset;
	int status;
	int ret;

	child = fork ();
	if (child == 0) {	/* child shell */
		/*
		 * PAM_DATA_SILENT is not supported by some modules, and
		 * there is no strong need to clean up the process space's
		 * memory since we will either call exec or exit.
		pam_end (pamh, PAM_SUCCESS | PAM_DATA_SILENT);
		 */

		if (doshell) {
			(void) shell (shellstr, (char *) args[0], envp);
		} else {
			/* There is no need for a controlling terminal.
			 * This avoids the callee to inject commands on
			 * the caller's tty. */
			(void) setsid ();

			execve_shell (shellstr, (char **) args, envp);
		}

		exit (errno == ENOENT ? E_CMD_NOTFOUND : E_CMD_NOEXEC);
	} else if ((pid_t)-1 == child) {
		(void) fprintf (stderr,
		                _("%s: Cannot fork user shell\n"),
		                Prog);
		SYSLOG ((LOG_WARN, "Cannot execute %s", shellstr));
		closelog ();
		exit (1);
	}
	/* parent only */
	pid_child = child;
	sigfillset (&ourset);
	if (sigprocmask (SIG_BLOCK, &ourset, NULL) != 0) {
		(void) fprintf (stderr,
		                _("%s: signal malfunction\n"),
		                Prog);
		caught = SIGTERM;
	}
	if (0 == caught) {
		struct sigaction action;

		action.sa_handler = catch_signals;
		sigemptyset (&action.sa_mask);
		action.sa_flags = 0;
		sigemptyset (&ourset);

		if (   (sigaddset (&ourset, SIGTERM) != 0)
		    || (sigaddset (&ourset, SIGALRM) != 0)
		    || (sigaction (SIGTERM, &action, NULL) != 0)
		    || (   !doshell /* handle SIGINT (Ctrl-C), SIGQUIT
		                     * (Ctrl-\), and SIGTSTP (Ctrl-Z)
		                     * since the child does not control
		                     * the tty anymore.
		                     */
		        && (   (sigaddset (&ourset, SIGINT)  != 0)
		            || (sigaddset (&ourset, SIGQUIT) != 0)
		            || (sigaddset (&ourset, SIGTSTP) != 0)
		            || (sigaction (SIGINT,  &action, NULL) != 0)
		            || (sigaction (SIGQUIT, &action, NULL) != 0))
		            || (sigaction (SIGTSTP,  &action, NULL) != 0))
		    || (sigprocmask (SIG_UNBLOCK, &ourset, NULL) != 0)
		    ) {
			fprintf (stderr,
			         _("%s: signal masking malfunction\n"),
			         Prog);
			caught = SIGTERM;
		}
	}

	if (0 == caught) {
		bool stop = true;

		do {
			pid_t pid;
			stop = true;

			pid = waitpid (-1, &status, WUNTRACED);

			/* When interrupted by signal, the signal will be
			 * forwarded to the child, and termination will be
			 * forced later.
			 */
			if (   ((pid_t)-1 == pid)
			    && (EINTR == errno)
			    && (SIGTSTP == caught)) {
				/* Except for SIGTSTP, which request to
				 * stop the child.
				 * We will SIGSTOP ourself on the next
				 * waitpid round.
				 */
				kill (child, SIGSTOP);
				stop = false;
			} else if (   ((pid_t)-1 != pid)
			           && (0 != WIFSTOPPED (status))) {
				/* The child (shell) was suspended.
				 * Suspend su. */
				kill (getpid (), SIGSTOP);
				/* wake child when resumed */
				kill (pid, SIGCONT);
				stop = false;
			}
		} while (!stop);
	}

	if (0 != caught) {
		(void) fputs ("\n", stderr);
		(void) fputs (_("Session terminated, terminating shell..."),
		              stderr);
		(void) kill (child, caught);
	}

	ret = pam_close_session (pamh, 0);
	if (PAM_SUCCESS != ret) {
		SYSLOG ((LOG_ERR, "pam_close_session: %s",
			 pam_strerror (pamh, ret)));
		fprintf (stderr, _("%s: %s\n"), Prog, pam_strerror (pamh, ret));
		(void) pam_end (pamh, ret);
		exit (1);
	}

	ret = pam_end (pamh, PAM_SUCCESS);

	if (0 != caught) {
		(void) signal (SIGALRM, kill_child);
		(void) alarm (2);

		(void) wait (&status);
		(void) fputs (_(" ...terminated.\n"), stderr);
	}

	exit ((0 != WIFEXITED (status)) ? WEXITSTATUS (status)
	                                : WTERMSIG (status) + 128);
}
#endif

/*
 * usage - print command line syntax and exit
  */
static void usage (int status)
{
	fputs (_("Usage: su [options] [LOGIN]\n"
	         "\n"
	         "Options:\n"
	         "  -c, --command COMMAND         pass COMMAND to the invoked shell\n"
	         "  -h, --help                    display this help message and exit\n"
	         "  -, -l, --login                make the shell a login shell\n"
	         "  -m, -p,\n"
	         "  --preserve-environment        do not reset environment variables, and\n"
	         "                                keep the same shell\n"
	         "  -s, --shell SHELL             use SHELL instead of the default in passwd\n"
	         "\n"), (E_SUCCESS != status) ? stderr : stdout);
	exit (status);
}

/*
 * su - switch user id
 *
 *	su changes the user's ids to the values for the specified user.  if
 *	no new user name is specified, "root" or UID 0 is used by default.
 *
 *	Any additional arguments are passed to the user's shell. In
 *	particular, the argument "-c" will cause the next argument to be
 *	interpreted as a command by the common shell programs.
 */
int main (int argc, char **argv)
{
	const char *cp;
	const char *tty = NULL;	/* Name of tty SU is run from        */
	bool doshell = false;
	bool fakelogin = false;
	bool amroot = false;
	uid_t my_uid;
	struct passwd *pw = NULL;
	char **envp = environ;
	char *shellstr = NULL;
	char *command = NULL;

#ifdef USE_PAM
	char **envcp;
	int ret;
#else				/* !USE_PAM */
	int err = 0;

	RETSIGTYPE (*oldsig) (int);
	int is_console = 0;

	struct spwd *spwd = 0;

#ifdef SU_ACCESS
	char *oldpass;
#endif
#endif				/* !USE_PAM */

	(void) setlocale (LC_ALL, "");
	(void) bindtextdomain (PACKAGE, LOCALEDIR);
	(void) textdomain (PACKAGE);

	change_environment = true;

	/*
	 * Get the program name. The program name is used as a prefix to
	 * most error messages.
	 */
	Prog = Basename (argv[0]);

	OPENLOG ("su");

	/*
	 * Process the command line arguments. 
	 */

	{
		/*
		 * Parse the command line options.
		 */
		int option_index = 0;
		int c;
		static struct option long_options[] = {
			{"command", required_argument, NULL, 'c'},
			{"help", no_argument, NULL, 'h'},
			{"login", no_argument, NULL, 'l'},
			{"preserve-environment", no_argument, NULL, 'p'},
			{"shell", required_argument, NULL, 's'},
			{NULL, 0, NULL, '\0'}
		};

		while ((c =
			getopt_long (argc, argv, "c:hlmps:", long_options,
				     &option_index)) != -1) {
			switch (c) {
			case 'c':
				command = optarg;
				break;
			case 'h':
				usage (E_SUCCESS);
				break;
			case 'l':
				fakelogin = true;
				break;
			case 'm':
			case 'p':
				/* This will only have an effect if the target
				 * user do not have a restricted shell, or if
				 * su is called by root.
				 */
				change_environment = false;
				break;
			case 's':
				shellstr = optarg;
				break;
			default:
				usage (E_USAGE);	/* NOT REACHED */
			}
		}

		if ((optind < argc) && (strcmp (argv[optind], "-") == 0)) {
			fakelogin = true;
			optind++;
			if (   (optind < argc)
			    && (strcmp (argv[optind], "--") == 0)) {
				optind++;
			}
		}
	}

	initenv ();

	my_uid = getuid ();
	amroot = (my_uid == 0);

	/*
	 * Get the tty name. Entries will be logged indicating that the user
	 * tried to change to the named new user from the current terminal.
	 */
	tty = ttyname (0);
	if ((isatty (0) != 0) && (NULL != tty)) {
#ifndef USE_PAM
		is_console = console (tty);
#endif
	} else {
		/*
		 * Be more paranoid, like su from SimplePAMApps.  --marekm
		 */
		if (!amroot) {
			fprintf (stderr,
			         _("%s: must be run from a terminal\n"),
			         Prog);
			exit (1);
		}
		tty = "???";
	}

	/*
	 * The next argument must be either a user ID, or some flag to a
	 * subshell. Pretty sticky since you can't have an argument which
	 * doesn't start with a "-" unless you specify the new user name.
	 * Any remaining arguments will be passed to the user's login shell.
	 */
	if ((optind < argc) && ('-' != argv[optind][0])) {
		STRFCPY (name, argv[optind++]);	/* use this login id */
		if ((optind < argc) && (strcmp (argv[optind], "--") == 0)) {
			optind++;
		}
	}
	if ('\0' == name[0]) {		/* use default user */
		struct passwd *root_pw = getpwnam ("root");
		if ((NULL != root_pw) && (0 == root_pw->pw_uid)) {
			(void) strcpy (name, "root");
		} else {
			root_pw = getpwuid (0);
			if (NULL == root_pw) {
				SYSLOG ((LOG_CRIT, "There is no UID 0 user."));
				su_failure (tty);
			}
			(void) strcpy (name, root_pw->pw_name);
		}
	}

	doshell = (argc == optind);	/* any arguments remaining? */
	if (NULL != command) {
		doshell = false;
	}

	/*
	 * Get the user's real name. The current UID is used to determine
	 * who has executed su. That user ID must exist.
	 */
	pw = get_my_pwent ();
	if (NULL == pw) {
		fprintf (stderr,
		         _("%s: Cannot determine your user name.\n"),
		         Prog);
		SYSLOG ((LOG_WARN, "Cannot determine the user name of the caller (UID %lu)",
		         (unsigned long) my_uid));
		su_failure (tty);
	}
	STRFCPY (oldname, pw->pw_name);

#ifndef USE_PAM
#ifdef SU_ACCESS
	/*
	 * Sort out the password of user calling su, in case needed later
	 * -- chris
	 */
	spwd = getspnam (oldname); /* !USE_PAM, no need for xgetspnam */
	if (NULL != spwd) {
		pw->pw_passwd = spwd->sp_pwdp;
	}
	oldpass = xstrdup (pw->pw_passwd);
#endif				/* SU_ACCESS */

#else				/* USE_PAM */
	ret = pam_start ("su", name, &conv, &pamh);
	if (PAM_SUCCESS != ret) {
		SYSLOG ((LOG_ERR, "pam_start: error %d", ret);
			fprintf (stderr,
			         _("%s: pam_start: error %d\n"),
			         Prog, ret));
		exit (1);
	}

	ret = pam_set_item (pamh, PAM_TTY, (const void *) tty);
	if (PAM_SUCCESS == ret) {
		ret = pam_set_item (pamh, PAM_RUSER, (const void *) oldname);
	}
	if (PAM_SUCCESS != ret) {
		SYSLOG ((LOG_ERR, "pam_set_item: %s",
			 pam_strerror (pamh, ret)));
		fprintf (stderr, _("%s: %s\n"), Prog, pam_strerror (pamh, ret));
		pam_end (pamh, ret);
		exit (1);
	}
#endif				/* USE_PAM */

      top:
	/*
	 * This is the common point for validating a user whose name is
	 * known. It will be reached either by normal processing, or if the
	 * user is to be logged into a subsystem root.
	 *
	 * The password file entries for the user is gotten and the account
	 * validated.
	 */
	pw = xgetpwnam (name);
	if (NULL == pw) {
		(void) fprintf (stderr, _("Unknown id: %s\n"), name);
		closelog ();
		exit (1);
	}
#ifndef USE_PAM
	spwd = NULL;
	if (strcmp (pw->pw_passwd, SHADOW_PASSWD_STRING) == 0) {
		spwd = getspnam (name); /* !USE_PAM, no need for xgetspnam */
		if (NULL != spwd) {
			pw->pw_passwd = spwd->sp_pwdp;
		}
	}
#endif				/* !USE_PAM */
	pwent = *pw;

	/* If su is not called by root, and the target user has a restricted
	 * shell, the environment must be changed.
	 */
	change_environment |= (restricted_shell (pwent.pw_shell) && !amroot);

	/*
	 * If a new login is being set up, the old environment will be
	 * ignored and a new one created later on.
	 * (note: in the case of a subsystem, the shell will be restricted,
	 *        and this won't be executed on the first pass)
	 */
	if (change_environment && fakelogin) {
		/*
		 * The terminal type will be left alone if it is present in
		 * the environment already.
		 */
		cp = getenv ("TERM");
		if (NULL != cp) {
			addenv ("TERM", cp);
		}

		/*
		 * For some terminals COLORTERM seems to be the only way
		 * for checking for that specific terminal. For instance,
		 * gnome-terminal sets its TERM as "xterm" but its
		 * COLORTERM as "gnome-terminal". The COLORTERM variable
		 * is also of use when running GNU screen since it sets
		 * TERM to "screen" but doesn't touch COLORTERM.
		 */
		cp = getenv ("COLORTERM");
		if (NULL != cp) {
			addenv ("COLORTERM", cp);
		}

#ifndef USE_PAM
		cp = getdef_str ("ENV_TZ");
		if (NULL != cp) {
			addenv (('/' == *cp) ? tz (cp) : cp, NULL);
		}

		/*
		 * The clock frequency will be reset to the login value if required
		 */
		cp = getdef_str ("ENV_HZ");
		if (NULL != cp) {
			addenv (cp, NULL);	/* set the default $HZ, if one */
		}
#endif				/* !USE_PAM */

		/*
		 * Also leave DISPLAY and XAUTHORITY if present, else
		 * pam_xauth will not work.
		 */
		cp = getenv ("DISPLAY");
		if (NULL != cp) {
			addenv ("DISPLAY", cp);
		}
		cp = getenv ("XAUTHORITY");
		if (NULL != cp) {
			addenv ("XAUTHORITY", cp);
		}
	} else {
		while (NULL != *envp) {
			addenv (*envp, NULL);
			envp++;
		}
	}

#ifndef USE_PAM
	/*
	 * BSD systems only allow "wheel" to SU to root. USG systems don't,
	 * so we make this a configurable option.
	 */

	/* The original Shadow 3.3.2 did this differently. Do it like BSD:
	 *
	 * - check for UID 0 instead of name "root" - there are systems with
	 *   several root accounts under different names,
	 *
	 * - check the contents of /etc/group instead of the current group
	 *   set (you must be listed as a member, GID 0 is not sufficient).
	 *
	 * In addition to this traditional feature, we now have complete su
	 * access control (allow, deny, no password, own password).  Thanks
	 * to Chris Evans <lady0110@sable.ox.ac.uk>.
	 */

	if (!amroot) {
		if (   (0 == pwent.pw_uid)
		    && getdef_bool ("SU_WHEEL_ONLY")
		    && !iswheel (oldname)) {
			fprintf (stderr,
			         _("You are not authorized to su %s\n"),
			         name);
			exit (1);
		}
#ifdef SU_ACCESS
		switch (check_su_auth (oldname, name)) {
		case 0:	/* normal su, require target user's password */
			break;
		case 1:	/* require no password */
			pwent.pw_passwd = "";	/* XXX warning: const */
			break;
		case 2:	/* require own password */
			puts (_("(Enter your own password)"));
			pwent.pw_passwd = oldpass;
			break;
		default:	/* access denied (-1) or unexpected value */
			fprintf (stderr,
			         _("You are not authorized to su %s\n"),
			         name);
			exit (1);
		}
#endif				/* SU_ACCESS */
	}
#endif				/* !USE_PAM */

	/* If the user do not want to change the environment,
	 * use the current SHELL.
	 * (unless another shell is required by the command line)
	 */
	if ((NULL == shellstr) && !change_environment) {
		shellstr = getenv ("SHELL");
	}
	/* For users with non null UID, if this user has a restricted
	 * shell, the shell must be the one specified in /etc/passwd
	 */
	if (   (NULL != shellstr)
	    && !amroot
	    && restricted_shell (pwent.pw_shell)) {
		shellstr = NULL;
	}
	/* If the shell is not set at this time, use the shell specified
	 * in /etc/passwd.
	 */
	if (NULL == shellstr) {
		shellstr = (char *) strdup (pwent.pw_shell);
	}

	/*
	 * Set the default shell.
	 */
	if ((NULL == shellstr) || ('\0' == shellstr[0])) {
		shellstr = SHELL;
	}

	(void) signal (SIGINT, SIG_IGN);
	(void) signal (SIGQUIT, SIG_IGN);
#ifdef USE_PAM
	ret = pam_authenticate (pamh, 0);
	if (PAM_SUCCESS != ret) {
		SYSLOG ((LOG_ERR, "pam_authenticate: %s",
			 pam_strerror (pamh, ret)));
		fprintf (stderr, _("%s: %s\n"), Prog, pam_strerror (pamh, ret));
		(void) pam_end (pamh, ret);
		su_failure (tty);
	}

	ret = pam_acct_mgmt (pamh, 0);
	if (PAM_SUCCESS != ret) {
		if (amroot) {
			fprintf (stderr,
			         _("%s: %s\n(Ignored)\n"),
			         Prog, pam_strerror (pamh, ret));
		} else if (PAM_NEW_AUTHTOK_REQD == ret) {
			ret = pam_chauthtok (pamh, PAM_CHANGE_EXPIRED_AUTHTOK);
			if (PAM_SUCCESS != ret) {
				SYSLOG ((LOG_ERR, "pam_chauthtok: %s",
					 pam_strerror (pamh, ret)));
				fprintf (stderr,
				         _("%s: %s\n"),
				         Prog, pam_strerror (pamh, ret));
				(void) pam_end (pamh, ret);
				su_failure (tty);
			}
		} else {
			SYSLOG ((LOG_ERR, "pam_acct_mgmt: %s",
				 pam_strerror (pamh, ret)));
			fprintf (stderr,
			         _("%s: %s\n"),
			         Prog, pam_strerror (pamh, ret));
			(void) pam_end (pamh, ret);
			su_failure (tty);
		}
	}
#else				/* !USE_PAM */
	/*
	 * Set up a signal handler in case the user types QUIT.
	 */
	die (0);
	oldsig = signal (SIGQUIT, die);

	/*
	 * See if the system defined authentication method is being used. 
	 * The first character of an administrator defined method is an '@'
	 * character.
	 */
	if (   !amroot
	    && (pw_auth (pwent.pw_passwd, name, PW_SU, (char *) 0) != 0)) {
		SYSLOG (((pwent.pw_uid != 0)? LOG_NOTICE : LOG_WARN,
		         "Authentication failed for %s", name));
		fprintf(stderr, _("%s: Authentication failure\n"), Prog);
		su_failure (tty);
	}
	(void) signal (SIGQUIT, oldsig);

	/*
	 * Check to see if the account is expired. root gets to ignore any
	 * expired accounts, but normal users can't become a user with an
	 * expired password.
	 */
	if ((!amroot) && (NULL != spwd)) {
		(void) expire (&pwent, spwd);
	}

	/*
	 * Check to see if the account permits "su". root gets to ignore any
	 * restricted accounts, but normal users can't become a user if
	 * there is a "SU" entry in the /etc/porttime file denying access to
	 * the account.
	 */
	if (!amroot) {
		if (!isttytime (pwent.pw_name, "SU", time ((time_t *) 0))) {
			SYSLOG (((0 != pwent.pw_uid) ? LOG_WARN : LOG_CRIT,
			         "SU by %s to restricted account %s",
			         oldname, name));
			fprintf (stderr,
			         _("%s: You are not authorized to su at that time\n"),
			         Prog);
			su_failure (tty);
		}
	}
#endif				/* !USE_PAM */

	(void) signal (SIGINT, SIG_DFL);
	(void) signal (SIGQUIT, SIG_DFL);

	cp = getdef_str ((pwent.pw_uid == 0) ? "ENV_SUPATH" : "ENV_PATH");
	if (NULL == cp) {
		addenv ((pwent.pw_uid == 0) ? "PATH=/sbin:/bin:/usr/sbin:/usr/bin" : "PATH=/bin:/usr/bin", NULL);
	} else if (strchr (cp, '=') != NULL) {
		addenv (cp, NULL);
	} else {
		addenv ("PATH", cp);
	}

	if (getenv ("IFS") != NULL) {	/* don't export user IFS ... */
		addenv ("IFS= \t\n", NULL);	/* ... instead, set a safe IFS */
	}

	/*
	 * Even if --shell is specified, the subsystem login test is based on
	 * the shell specified in /etc/passwd (not the one specified with
	 * --shell, which will be the one executed in the chroot later).
	 */
	if ('*' == pwent.pw_shell[0]) {	/* subsystem root required */
		pwent.pw_shell++;	/* skip the '*' */
		subsystem (&pwent);	/* figure out what to execute */
		endpwent ();
		endspent ();
		goto top;
	}

	sulog (tty, true, oldname, name);	/* save SU information */
	endpwent ();
	endspent ();
#ifdef USE_SYSLOG
	if (getdef_bool ("SYSLOG_SU_ENAB")) {
		SYSLOG ((LOG_INFO, "+ %s %s:%s", tty,
		         ('\0' != oldname[0]) ? oldname : "???",
		         ('\0' != name[0]) ? name : "???"));
	}
#endif

#ifdef USE_PAM
	/* set primary group id and supplementary groups */
	if (setup_groups (&pwent) != 0) {
		pam_end (pamh, PAM_ABORT);
		exit (1);
	}

	/*
	 * pam_setcred() may do things like resource limits, console groups,
	 * and much more, depending on the configured modules
	 */
	ret = pam_setcred (pamh, PAM_ESTABLISH_CRED);
	if (PAM_SUCCESS != ret) {
		SYSLOG ((LOG_ERR, "pam_setcred: %s", pam_strerror (pamh, ret)));
		fprintf (stderr, _("%s: %s\n"), Prog, pam_strerror (pamh, ret));
		(void) pam_end (pamh, ret);
		exit (1);
	}

	ret = pam_open_session (pamh, 0);
	if (PAM_SUCCESS != ret) {
		SYSLOG ((LOG_ERR, "pam_open_session: %s",
			 pam_strerror (pamh, ret)));
		fprintf (stderr, _("%s: %s\n"), Prog, pam_strerror (pamh, ret));
		pam_setcred (pamh, PAM_DELETE_CRED);
		(void) pam_end (pamh, ret);
		exit (1);
	}

	/* we need to setup the environment *after* pam_open_session(),
	 * else the UID is changed before stuff like pam_xauth could
	 * run, and we cannot access /etc/shadow and co
	 */
	environ = newenvp;	/* make new environment active */

	if (change_environment) {
		/* update environment with all pam set variables */
		envcp = pam_getenvlist (pamh);
		if (NULL != envcp) {
			while (NULL != *envcp) {
				addenv (*envcp, NULL);
				envcp++;
			}
		}
	}

	/* become the new user */
	if (change_uid (&pwent) != 0) {
		pam_close_session (pamh, 0);
		pam_setcred (pamh, PAM_DELETE_CRED);
		(void) pam_end (pamh, PAM_ABORT);
		exit (1);
	}
#else				/* !USE_PAM */
	environ = newenvp;	/* make new environment active */

	/* no limits if su from root (unless su must fake login's behavior) */
	if (!amroot || fakelogin) {
		setup_limits (&pwent);
	}

	if (setup_uid_gid (&pwent, is_console) != 0) {
		exit (1);
	}
#endif				/* !USE_PAM */

	if (change_environment) {
		if (fakelogin) {
			pwent.pw_shell = shellstr;
			setup_env (&pwent);
		} else {
			addenv ("HOME", pwent.pw_dir);
			addenv ("USER", pwent.pw_name);
			addenv ("LOGNAME", pwent.pw_name);
			addenv ("SHELL", shellstr);
		}
	}

	/*
	 * This is a workaround for Linux libc bug/feature (?) - the
	 * /dev/log file descriptor is open without the close-on-exec flag
	 * and used to be passed to the new shell. There is "fcntl(LogFile,
	 * F_SETFD, 1)" in libc/misc/syslog.c, but it is commented out (at
	 * least in 5.4.33). Why?  --marekm
	 */
	closelog ();

	/*
	 * See if the user has extra arguments on the command line. In that
	 * case they will be provided to the new user's shell as arguments.
	 */
	if (fakelogin) {
		char *arg0;

		cp = getdef_str ("SU_NAME");
		if (NULL == cp) {
			cp = Basename (shellstr);
		}

		arg0 = xmalloc (strlen (cp) + 2);
		arg0[0] = '-';
		strcpy (arg0 + 1, cp);
		cp = arg0;
	} else {
		cp = Basename (shellstr);
	}

	if (!doshell) {
		/* Position argv to the remaining arguments */
		argv += optind;
		if (NULL != command) {
			argv -= 2;
			argv[0] = "-c";
			argv[1] = command;
		}
		/*
		 * Use the shell and create an argv
		 * with the rest of the command line included.
		 */
		argv[-1] = cp;
#ifndef USE_PAM
		execve_shell (shellstr, &argv[-1], environ);
		err = errno;
		(void) fputs (_("No shell\n"), stderr);
		SYSLOG ((LOG_WARN, "Cannot execute %s", shellstr));
		closelog ();
		exit ((ENOENT == err) ? E_CMD_NOTFOUND : E_CMD_NOEXEC);
#else
		run_shell (shellstr, &argv[-1], false, environ); /* no return */
#endif
	}
#ifndef USE_PAM
	err = shell (shellstr, cp, environ);
	exit ((ENOENT == err) ? E_CMD_NOTFOUND : E_CMD_NOEXEC);
#else
	run_shell (shellstr, &cp, true, environ);
#endif
	/* NOT REACHED */
	exit (1);
}

