/*
 * Copyright 1989 - 1994, Julianne Frances Haugh
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
 * ARE DISCLAIMED. IN NO EVENT SHALL JULIE HAUGH OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <config.h>

#ident "$Id: su.c,v 1.70 2006/05/07 17:26:59 kloczek Exp $"

#include <getopt.h>
#include <grp.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include "prototypes.h"
#include "defines.h"
#include "exitcodes.h"
#include "pwauth.h"
#include "getdef.h"
#ifdef USE_PAM
#include "pam_defs.h"
#endif
/*
 * Assorted #defines to control su's behavior
 */
/*
 * Global variables
 */
/* not needed by sulog.c anymore */
static char name[BUFSIZ];
static char oldname[BUFSIZ];

/* If nonzero, change some environment vars to indicate the user su'd to. */
static int change_environment;

#ifdef USE_PAM
static pam_handle_t *pamh = NULL;
static int caught = 0;
#endif

static char *Prog;
extern struct passwd pwent;

/*
 * External identifiers
 */

extern char **newenvp;
extern char **environ;
extern size_t newenvc;

/* local function prototypes */

#ifndef USE_PAM

static RETSIGTYPE die (int);
static int iswheel (const char *);

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

	if (killed)
		STTY (0, &sgtty);
	else
		GTTY (0, &sgtty);

	if (killed) {
		closelog ();
		exit (killed);
	}
}

static int iswheel (const char *username)
{
	struct group *grp;

	grp = getgrnam ("wheel");;
	if (!grp || !grp->gr_mem)
		return 0;
	return is_on_list (grp->gr_mem, username);
}
#endif				/* !USE_PAM */

/* borrowed from GNU sh-utils' "su.c" */
static int restricted_shell (const char *shellstr)
{
	char *line;

	setusershell ();
	while ((line = getusershell ()) != NULL) {
		if (*line != '#' && strcmp (line, shellstr) == 0) {
			endusershell ();
			return 0;
		}
	}
	endusershell ();
	return 1;
}

static void su_failure (const char *tty)
{
	sulog (tty, 0, oldname, name);	/* log failed attempt */
#ifdef USE_SYSLOG
	if (getdef_bool ("SYSLOG_SU_ENAB"))
		SYSLOG ((pwent.pw_uid ? LOG_INFO : LOG_NOTICE,
			 "- %s %s:%s", tty,
			 oldname[0] ? oldname : "???", name[0] ? name : "???"));
	closelog ();
#endif
	puts (_("Sorry."));
	exit (1);
}


#ifdef USE_PAM
/* Signal handler for parent process later */
static void catch_signals (int sig)
{
	++caught;
}

/* This I ripped out of su.c from sh-utils after the Mandrake pam patch
 * have been applied.  Some work was needed to get it integrated into
 * su.c from shadow.
 */
static void run_shell (const char *shellstr, char *args[], int doshell,
		       char *const envp[])
{
	int child;
	sigset_t ourset;
	int status;
	int ret;

	child = fork ();
	if (child == 0) {	/* child shell */
		pam_end (pamh, PAM_SUCCESS);

		if (doshell)
			(void) shell (shellstr, (char *) args[0], envp);
		else
			(void) execve (shellstr, (char **) args, envp);
		exit (errno == ENOENT ? E_CMD_NOTFOUND : E_CMD_NOEXEC);
	} else if (child == -1) {
		(void) fprintf (stderr, "%s: Cannot fork user shell\n", Prog);
		SYSLOG ((LOG_WARN, "Cannot execute %s", shellstr));
		closelog ();
		exit (1);
	}
	/* parent only */
	sigfillset (&ourset);
	if (sigprocmask (SIG_BLOCK, &ourset, NULL)) {
		(void) fprintf (stderr, "%s: signal malfunction\n", Prog);
		caught = 1;
	}
	if (!caught) {
		struct sigaction action;

		action.sa_handler = catch_signals;
		sigemptyset (&action.sa_mask);
		action.sa_flags = 0;
		sigemptyset (&ourset);

		if (sigaddset (&ourset, SIGTERM)
		    || sigaddset (&ourset, SIGALRM)
		    || sigaction (SIGTERM, &action, NULL)
		    || sigprocmask (SIG_UNBLOCK, &ourset, NULL)
		    ) {
			fprintf (stderr,
				 "%s: signal masking malfunction\n", Prog);
			caught = 1;
		}
	}

	if (!caught) {
		do {
			int pid;

			pid = waitpid (-1, &status, WUNTRACED);

			if (WIFSTOPPED (status)) {
				kill (getpid (), SIGSTOP);
				/* once we get here, we must have resumed */
				kill (pid, SIGCONT);
			}
		} while (WIFSTOPPED (status));
	}

	if (caught) {
		fprintf (stderr, "\nSession terminated, killing shell...");
		kill (child, SIGTERM);
	}

	ret = pam_close_session (pamh, 0);
	if (ret != PAM_SUCCESS) {
		SYSLOG ((LOG_ERR, "pam_close_session: %s",
			 pam_strerror (pamh, ret)));
		fprintf (stderr, _("%s: %s\n"), Prog, pam_strerror (pamh, ret));
		pam_end (pamh, ret);
		exit (1);
	}

	ret = pam_end (pamh, PAM_SUCCESS);

	if (caught) {
		sleep (2);
		kill (child, SIGKILL);
		fprintf (stderr, " ...killed.\n");
		exit (-1);
	}

	exit (WIFEXITED (status)
	      ? WEXITSTATUS (status)
	      : WTERMSIG (status) + 128);
}
#endif

/*
 * usage - print command line syntax and exit
  */
static void usage (void)
{
	fprintf (stderr, _("Usage: su [options] [login]\n"
			   "\n"
			   "Options:\n"
			   "  -c, --command COMMAND		pass COMMAND to the invoked shell\n"
			   "  -h, --help			display this help message and exit\n"
			   "  -, -l, --login		make the shell a login shell\n"
			   "  -m, -p,\n"
			   "  --preserve-environment	do not reset environment variables, and keep\n"
			   "				the same shell\n"
			   "  -s, --shell SHELL		use SHELL instead of the default in passwd\n"));
	exit (E_USAGE);
}

/*
 * su - switch user id
 *
 *	su changes the user's ids to the values for the specified user.  if
 *	no new user name is specified, "root" is used by default.
 *
 *	Any additional arguments are passed to the user's shell. In
 *	particular, the argument "-c" will cause the next argument to be
 *	interpreted as a command by the common shell programs.
 */
int main (int argc, char **argv)
{
	char *cp;
	char **envcp;
	const char *tty = 0;	/* Name of tty SU is run from        */
	int doshell = 0;
	int fakelogin = 0;
	int amroot = 0;
	uid_t my_uid;
	struct passwd *pw = 0;
	char **envp = environ;
	char *shellstr = 0, *command = 0;

#ifdef USE_PAM
	int ret;
#else				/* !USE_PAM */
	int err = 0;

	RETSIGTYPE (*oldsig) ();
	int is_console = 0;

	struct spwd *spwd = 0;

#ifdef SU_ACCESS
	char *oldpass;
#endif
#endif				/* !USE_PAM */

	sanitize_env ();

	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);

	change_environment = 1;

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
			getopt_long (argc, argv, "-c:hlmps:", long_options,
				     &option_index)) != -1) {
			switch (c) {
			case 1:
				/* this is not an su option */
				/* The next arguments are either '-', the
				 * target name, or arguments to be passed
				 * to the shell.
				 */
				/* rewind the (not yet handled) option */
				optind--;
				goto end_su_options;
				break;	/* NOT REACHED */
			case 'c':
				command = optarg;
				break;
			case 'h':
				usage ();
				break;
			case 'l':
				fakelogin = 1;
				break;
			case 'm':
			case 'p':
				/* This will only have an effect if the target
				 * user do not have a restricted shell, or if
				 * su is called by root.
				 */
				change_environment = 0;
				break;
			case 's':
				shellstr = optarg;
				break;
			default:
				usage ();	/* NOT REACHED */
			}
		}
	      end_su_options:
		if (optind < argc && !strcmp (argv[optind], "-")) {
			fakelogin = 1;
			optind++;
			if (optind < argc && !strcmp (argv[optind], "--"))
				optind++;
		}
	}

	initenv ();

	my_uid = getuid ();
	amroot = (my_uid == 0);

	/*
	 * Get the tty name. Entries will be logged indicating that the user
	 * tried to change to the named new user from the current terminal.
	 */
	if (isatty (0) && (cp = ttyname (0))) {
		if (strncmp (cp, "/dev/", 5) == 0)
			tty = cp + 5;
		else
			tty = cp;
#ifndef USE_PAM
		is_console = console (tty);
#endif
	} else {
		/*
		 * Be more paranoid, like su from SimplePAMApps.  --marekm
		 */
		if (!amroot) {
			fprintf (stderr,
				 _("%s: must be run from a terminal\n"), Prog);
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
	if (optind < argc && argv[optind][0] != '-') {
		STRFCPY (name, argv[optind++]);	/* use this login id */
		if (optind < argc && !strcmp (argv[optind], "--"))
			optind++;
	}
	if (!name[0])		/* use default user ID */
		(void) strcpy (name, "root");

	doshell = argc == optind;	/* any arguments remaining? */
	if (command)
		doshell = 0;

	/*
	 * Get the user's real name. The current UID is used to determine
	 * who has executed su. That user ID must exist.
	 */
	pw = get_my_pwent ();
	if (!pw) {
		SYSLOG ((LOG_CRIT, "Unknown UID: %u", my_uid));
		su_failure (tty);
	}
	STRFCPY (oldname, pw->pw_name);

#ifndef USE_PAM
#ifdef SU_ACCESS
	/*
	 * Sort out the password of user calling su, in case needed later
	 * -- chris
	 */
	if ((spwd = getspnam (oldname)))
		pw->pw_passwd = spwd->sp_pwdp;
	oldpass = xstrdup (pw->pw_passwd);
#endif				/* SU_ACCESS */

#else				/* USE_PAM */
	ret = pam_start ("su", name, &conv, &pamh);
	if (ret != PAM_SUCCESS) {
		SYSLOG ((LOG_ERR, "pam_start: error %d", ret);
			fprintf (stderr, _("%s: pam_start: error %d\n"),
				 Prog, ret));
		exit (1);
	}

	ret = pam_set_item (pamh, PAM_TTY, (const void *) tty);
	if (ret == PAM_SUCCESS)
		ret = pam_set_item (pamh, PAM_RUSER, (const void *) oldname);
	if (ret != PAM_SUCCESS) {
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
	if (!(pw = getpwnam (name))) {
		(void) fprintf (stderr, _("Unknown id: %s\n"), name);
		closelog ();
		exit (1);
	}
#ifndef USE_PAM
	spwd = NULL;
	if (strcmp (pw->pw_passwd, SHADOW_PASSWD_STRING) == 0
	    && (spwd = getspnam (name)))
		pw->pw_passwd = spwd->sp_pwdp;
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
	if (fakelogin && change_environment) {
		/*
		 * The terminal type will be left alone if it is present in
		 * the environment already.
		 */
		if ((cp = getenv ("TERM")))
			addenv ("TERM", cp);
#ifndef USE_PAM
		if ((cp = getdef_str ("ENV_TZ")))
			addenv (*cp == '/' ? tz (cp) : cp, NULL);

		/*
		 * The clock frequency will be reset to the login value if required
		 */
		if ((cp = getdef_str ("ENV_HZ")))
			addenv (cp, NULL);	/* set the default $HZ, if one */

		/*
		 * Also leave DISPLAY and XAUTHORITY if present, else
		 * pam_xauth will not work.
		 */
		if ((cp = getenv ("DISPLAY")))
			addenv ("DISPLAY", cp);
		if ((cp = getenv ("XAUTHORITY")))
			addenv ("XAUTHORITY", cp);
#endif				/* !USE_PAM */
	} else {
		while (*envp)
			addenv (*envp++, NULL);
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
		if (pwent.pw_uid == 0 && getdef_bool ("SU_WHEEL_ONLY")
		    && !iswheel (oldname)) {
			fprintf (stderr,
				 _("You are not authorized to su %s\n"), name);
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
				 _("You are not authorized to su %s\n"), name);
			exit (1);
		}
#endif				/* SU_ACCESS */
	}
#endif				/* !USE_PAM */

	/* If the user do not want to change the environment,
	 * use the current SHELL.
	 * (unless another shell is required by the command line)
	 */
	if (shellstr == NULL && change_environment == 0)
		shellstr = getenv ("SHELL");
	/* For users with non null UID, if this user has a restricted
	 * shell, the shell must be the one specified in /etc/passwd
	 */
	if (shellstr != NULL && !amroot && restricted_shell (pwent.pw_shell))
		shellstr = NULL;
	/* If the shell is not set at this time, use the shell specified
	 * in /etc/passwd.
	 */
	if (shellstr == NULL)
		shellstr = (char *) strdup (pwent.pw_shell);

	/*
	 * Set the default shell.
	 */
	if (shellstr == NULL || shellstr[0] == '\0')
		shellstr = "/bin/sh";

	signal (SIGINT, SIG_IGN);
	signal (SIGQUIT, SIG_IGN);
#ifdef USE_PAM
	ret = pam_authenticate (pamh, 0);
	if (ret != PAM_SUCCESS) {
		SYSLOG ((LOG_ERR, "pam_authenticate: %s",
			 pam_strerror (pamh, ret)));
		fprintf (stderr, _("%s: %s\n"), Prog, pam_strerror (pamh, ret));
		pam_end (pamh, ret);
		su_failure (tty);
	}

	ret = pam_acct_mgmt (pamh, 0);
	if (ret != PAM_SUCCESS) {
		if (amroot) {
			fprintf (stderr, _("%s: %s\n(Ignored)\n"), Prog,
				 pam_strerror (pamh, ret));
		} else if (ret == PAM_NEW_AUTHTOK_REQD) {
			ret = pam_chauthtok (pamh, PAM_CHANGE_EXPIRED_AUTHTOK);
			if (ret != PAM_SUCCESS) {
				SYSLOG ((LOG_ERR, "pam_chauthtok: %s",
					 pam_strerror (pamh, ret)));
				fprintf (stderr, _("%s: %s\n"), Prog,
					 pam_strerror (pamh, ret));
				pam_end (pamh, ret);
				su_failure (tty);
			}
		} else {
			SYSLOG ((LOG_ERR, "pam_acct_mgmt: %s",
				 pam_strerror (pamh, ret)));
			fprintf (stderr, _("%s: %s\n"), Prog,
				 pam_strerror (pamh, ret));
			pam_end (pamh, ret);
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
	if (!amroot && pw_auth (pwent.pw_passwd, name, PW_SU, (char *) 0)) {
		SYSLOG ((pwent.pw_uid ? LOG_NOTICE : LOG_WARN,
			 "Authentication failed for %s", name));
		su_failure (tty);
	}
	signal (SIGQUIT, oldsig);

	/*
	 * Check to see if the account is expired. root gets to ignore any
	 * expired accounts, but normal users can't become a user with an
	 * expired password.
	 */
	if (!amroot) {
		if (!spwd)
			spwd = pwd_to_spwd (&pwent);

		if (expire (&pwent, spwd)) {
			struct passwd *pwd = getpwnam (name);

			spwd = getspnam (name);
			if (pwd)
				pwent = *pwd;
		}
	}

	/*
	 * Check to see if the account permits "su". root gets to ignore any
	 * restricted accounts, but normal users can't become a user if
	 * there is a "SU" entry in the /etc/porttime file denying access to
	 * the account.
	 */
	if (!amroot) {
		if (!isttytime (pwent.pw_name, "SU", time ((time_t *) 0))) {
			SYSLOG ((pwent.pw_uid ? LOG_WARN : LOG_CRIT,
				 "SU by %s to restricted account %s",
				 oldname, name));
			su_failure (tty);
		}
	}
#endif				/* !USE_PAM */

	signal (SIGINT, SIG_DFL);
	signal (SIGQUIT, SIG_DFL);

	cp = getdef_str ((pwent.pw_uid == 0) ? "ENV_SUPATH" : "ENV_PATH");
	if (!cp) {
		addenv ("PATH=/bin:/usr/bin", NULL);
	} else if (strchr (cp, '=')) {
		addenv (cp, NULL);
	} else {
		addenv ("PATH", cp);
	}

#ifndef USE_PAM
	/* setup the environment for PAM later on, else we run into auth problems */
	environ = newenvp;	/* make new environment active */
#endif

	if (getenv ("IFS"))	/* don't export user IFS ... */
		addenv ("IFS= \t\n", NULL);	/* ... instead, set a safe IFS */

	/*
	 * Even if --shell is specified, the subsystem login test is based on
	 * the shell specified in /etc/passwd (not the one specified with
	 * --shell, which will be the one executed in the chroot later).
	 */
	if (pwent.pw_shell[0] == '*') {	/* subsystem root required */
		pwent.pw_shell++;	/* skip the '*' */
		subsystem (&pwent);	/* figure out what to execute */
		endpwent ();
		endspent ();
		goto top;
	}

	sulog (tty, 1, oldname, name);	/* save SU information */
	endpwent ();
	endspent ();
#ifdef USE_SYSLOG
	if (getdef_bool ("SYSLOG_SU_ENAB"))
		SYSLOG ((LOG_INFO, "+ %s %s:%s", tty,
			 oldname[0] ? oldname : "???", name[0] ? name : "???"));
#endif

#ifdef USE_PAM
	/* set primary group id and supplementary groups */
	if (setup_groups (&pwent)) {
		pam_end (pamh, PAM_ABORT);
		exit (1);
	}

	/*
	 * pam_setcred() may do things like resource limits, console groups,
	 * and much more, depending on the configured modules
	 */
	ret = pam_setcred (pamh, PAM_ESTABLISH_CRED);
	if (ret != PAM_SUCCESS) {
		SYSLOG ((LOG_ERR, "pam_setcred: %s", pam_strerror (pamh, ret)));
		fprintf (stderr, _("%s: %s\n"), Prog, pam_strerror (pamh, ret));
		pam_end (pamh, ret);
		exit (1);
	}

	ret = pam_open_session (pamh, 0);
	if (ret != PAM_SUCCESS) {
		SYSLOG ((LOG_ERR, "pam_open_session: %s",
			 pam_strerror (pamh, ret)));
		fprintf (stderr, _("%s: %s\n"), Prog, pam_strerror (pamh, ret));
		pam_setcred (pamh, PAM_DELETE_CRED);
		pam_end (pamh, ret);
		exit (1);
	}

	if (change_environment) {
		/* we need to setup the environment *after* pam_open_session(),
		 * else the UID is changed before stuff like pam_xauth could
		 * run, and we cannot access /etc/shadow and co
		 */
		environ = newenvp;	/* make new environment active */

		/* update environment with all pam set variables */
		envcp = pam_getenvlist (pamh);
		if (envcp) {
			while (*envcp) {
				addenv (*envcp, NULL);
				envcp++;
			}
		}
	}

	/* become the new user */
	if (change_uid (&pwent)) {
		pam_close_session (pamh, 0);
		pam_setcred (pamh, PAM_DELETE_CRED);
		pam_end (pamh, PAM_ABORT);
		exit (1);
	}
#else				/* !USE_PAM */
	if (!amroot)		/* no limits if su from root */
		setup_limits (&pwent);

	if (setup_uid_gid (&pwent, is_console))
		exit (1);
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
		if (!cp)
			cp = Basename (shellstr);

		arg0 = xmalloc (strlen (cp) + 2);
		arg0[0] = '-';
		strcpy (arg0 + 1, cp);
		cp = arg0;
	} else
		cp = Basename (shellstr);

	if (!doshell) {
		/* Position argv to the remaining arguments */
		argv += optind;
		if (command) {
			argv -= 2;
			argv[0] = "-c";
			argv[1] = command;
		}
		/*
		 * Use the shell and create an argv
		 * with the rest of the command line included.
		 */
		argv[-1] = shellstr;
#ifndef USE_PAM
		(void) execve (shellstr, &argv[-1], environ);
		err = errno;
		(void) fprintf (stderr, _("No shell\n"));
		SYSLOG ((LOG_WARN, "Cannot execute %s", shellstr));
		closelog ();
		exit (err == ENOENT ? E_CMD_NOTFOUND : E_CMD_NOEXEC);
#else
		run_shell (shellstr, &argv[-1], 0, environ);	/* no return */
#endif
	}
#ifndef USE_PAM
	err = shell (shellstr, cp, environ);
	exit (err == ENOENT ? E_CMD_NOTFOUND : E_CMD_NOEXEC);
#else
	run_shell (shellstr, &cp, 1, environ);
#endif
	/* NOT REACHED */
	exit (1);
}
