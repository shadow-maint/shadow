/*
 * SPDX-FileCopyrightText: 1989 - 1994, Julianne Frances Haugh
 * SPDX-FileCopyrightText: 1996 - 2000, Marek Michałkiewicz
 * SPDX-FileCopyrightText: 2001 - 2006, Tomasz Kłoczko
 * SPDX-FileCopyrightText: 2007 - 2011, Nicolas François
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#ident "$Id$"

#include <fcntl.h>
#include <getopt.h>
#include <pwd.h>
#include <stdio.h>
#include <sys/types.h>

#include "chkname.h"
#include "defines.h"
/*@-exitarg@*/
#include "exitcodes.h"
#include "fields.h"
#include "getdef.h"
#include "nscd.h"
#include "prototypes.h"
#include "pwauth.h"
#include "pwio.h"
#ifdef USE_PAM
#include "pam_defs.h"
#endif
#include "shadowlog.h"
#include "sssd.h"
#include "string/strcmp/streq.h"
#include "string/strcpy/strtcpy.h"
#include "string/strdup/strdup.h"


#ifndef SHELLS_FILE
#define SHELLS_FILE "/etc/shells"
#endif

#ifdef HAVE_VENDORDIR
#include <libeconf.h>
#define SHELLS "shells"
#define ETCDIR "/etc"
#endif

struct option_flags {
	bool chroot;
};

/*
 * Global variables
 */
static const char Prog[] = "chsh";	/* Program name */
static bool amroot;		/* Real UID is root */
static char loginsh[BUFSIZ];	/* Name of new login shell */
/* command line options */
static bool sflg = false;	/* -s - set shell from command line  */
static bool pw_locked = false;

/* external identifiers */

/* local function prototypes */
NORETURN static void fail_exit (int code, bool process_selinux);
NORETURN static void usage (int status);
static void new_fields (void);
static bool shell_is_listed (const char *, bool process_selinux);
static bool is_restricted_shell (const char *, bool process_selinux);
static void process_flags (int argc, char **argv, struct option_flags *flags);
static void check_perms(const struct passwd *pw, const struct option_flags *flags);
static void update_shell (const char *user, char *loginsh,
                          const struct option_flags *flags);

/*
 * fail_exit - do some cleanup and exit with the given error code
 */
NORETURN
static void
fail_exit (int code, bool process_selinux)
{
	if (pw_locked) {
		if (pw_unlock (process_selinux) == 0) {
			fprintf (stderr, _("%s: failed to unlock %s\n"), Prog, pw_dbname ());
			SYSLOG(LOG_ERR, "failed to unlock %s", pw_dbname());
			/* continue */
		}
	}

	closelog ();

	exit (code);
}

/*
 * usage - print command line syntax and exit
 */
NORETURN
static void
usage (int status)
{
	FILE *usageout = (E_SUCCESS != status) ? stderr : stdout;
	(void) fprintf (usageout,
	                _("Usage: %s [options] [LOGIN]\n"
	                  "\n"
	                  "Options:\n"),
	                Prog);
	(void) fputs (_("  -h, --help                    display this help message and exit\n"), usageout);
	(void) fputs (_("  -R, --root CHROOT_DIR         directory to chroot into\n"), usageout);
	(void) fputs (_("  -s, --shell SHELL             new login shell for the user account\n"), usageout);
	(void) fputs ("\n", usageout);
	exit (status);
}

/*
 * new_fields - change the user's login shell information interactively
 *
 * prompt the user for the login shell and change it according to the
 * response, or leave it alone if nothing was entered.
 */
static void new_fields (void)
{
	puts (_("Enter the new value, or press ENTER for the default"));
	change_field(loginsh, sizeof(loginsh), _("Login Shell"));
}

/*
 * is_restricted_shell - return true if the shell is restricted
 *
 */
static bool is_restricted_shell (const char *sh, bool process_selinux)
{
	/*
	 * Shells not listed in /etc/shells are considered to be restricted.
	 * Changed this to avoid confusion with "rc" (the plan9 shell - not
	 * restricted despite the name starting with 'r').  --marekm
	 */
	return !shell_is_listed (sh, process_selinux);
}

/*
 * shell_is_listed - see if the user's login shell is listed in /etc/shells
 *
 * The /etc/shells file is read for valid names of login shells.  If the
 * /etc/shells file does not exist the user cannot set any shell unless
 * they are root.
 *
 * If getusershell() is available (Linux, *BSD, possibly others), use it
 * instead of re-implementing it.
 */

#ifdef HAVE_VENDORDIR
static bool shell_is_listed (const char *sh, bool process_selinux)
{
	bool found = false;

	size_t size = 0;
	econf_err error;
	char **keys;
	econf_file *key_file;

	error = econf_readDirs(&key_file,
			       VENDORDIR,
			       ETCDIR,
			       SHELLS,
			       NULL,
			       "", /* key only */
			       "#" /* comment */);
	if (error) {
		fprintf (stderr,
			 _("Cannot parse shell files: %s"),
			 econf_errString(error));
		fail_exit (1, process_selinux);
	}

	error = econf_getKeys(key_file, NULL, &size, &keys);
	if (error) {
		fprintf (stderr,
			 _("Cannot evaluate entries in shell files: %s"),
			 econf_errString(error));
		econf_free (key_file);
		fail_exit (1, process_selinux);
	}

	for (size_t i = 0; i < size; i++) {
		if (streq(keys[i], sh)) {
			found = true;
			break;
		}
	}
	econf_free (keys);
	econf_free (key_file);

	return found;
}

#else /* without HAVE_VENDORDIR */

static bool shell_is_listed (const char *sh, bool)
{
	bool found = false;
	char *cp;

	setusershell ();
	while ((cp = getusershell ())) {
		if (streq(cp, sh)) {
			found = true;
			break;
		}
	}
	endusershell ();

	return found;
}
#endif /* with HAVE_VENDORDIR */

/*
 * process_flags - parse the command line options
 *
 *	It will not return if an error is encountered.
 */
static void process_flags (int argc, char **argv, struct option_flags *flags)
{
	int c;
	static struct option long_options[] = {
		{"help",  no_argument,       NULL, 'h'},
		{"root",  required_argument, NULL, 'R'},
		{"shell", required_argument, NULL, 's'},
		{NULL, 0, NULL, '\0'}
	};

	while ((c = getopt_long (argc, argv, "hR:s:",
	                         long_options, NULL)) != -1) {
		switch (c) {
		case 'h':
			usage (E_SUCCESS);
			/*@notreached@*/break;
		case 'R': /* no-op, handled in process_root_flag () */
			flags->chroot = true;
			break;
		case 's':
			sflg = true;
			strtcpy_a(loginsh, optarg);
			break;
		default:
			usage (E_USAGE);
		}
	}

	/*
	 * There should be only one remaining argument at most and it should
	 * be the user's name.
	 */
	if (argc > (optind + 1)) {
		usage (E_USAGE);
	}
}

/*
 * check_perms - check if the caller is allowed to add a group
 *
 *	Non-root users are only allowed to change their shell, if their current
 *	shell is not a restricted shell.
 *
 *	Non-root users must be authenticated.
 *
 *	It will not return if the user is not allowed.
 */
static void check_perms(const struct passwd *pw, const struct option_flags *flags)
{
#ifdef USE_PAM
	pam_handle_t *pamh = NULL;
	int retval;
	struct passwd *pampw;
#endif
	bool process_selinux;

	process_selinux = !flags->chroot;

	/*
	 * Non-privileged users are only allowed to change the shell if the
	 * UID of the user matches the current real UID.
	 */
	if (!amroot && pw->pw_uid != getuid ()) {
		SYSLOG(LOG_WARN, "can't change shell for '%s'", pw->pw_name);
		fprintf (stderr,
		         _("You may not change the shell for '%s'.\n"),
		         pw->pw_name);
		fail_exit (1, process_selinux);
	}

	/*
	 * Non-privileged users are only allowed to change the shell if it
	 * is not a restricted one.
	 */
	if (!amroot && is_restricted_shell (pw->pw_shell, process_selinux)) {
		SYSLOG(LOG_WARN, "can't change shell for '%s'", pw->pw_name);
		fprintf (stderr,
		         _("You may not change the shell for '%s'.\n"),
		         pw->pw_name);
		fail_exit (1, process_selinux);
	}
#ifdef WITH_SELINUX
	/*
	 * If the UID of the user does not match the current real UID,
	 * check if the change is allowed by SELinux policy.
	 */
	if ((pw->pw_uid != getuid ())
	    && (check_selinux_permit(Prog) != 0)) {
		SYSLOG(LOG_WARN, "can't change shell for '%s'", pw->pw_name);
		fprintf (stderr,
		         _("You may not change the shell for '%s'.\n"),
		         pw->pw_name);
		fail_exit (1, process_selinux);
	}
#endif

#ifndef USE_PAM
	/*
	 * Non-privileged users are optionally authenticated (must enter
	 * the password of the user whose information is being changed)
	 * before any changes can be made. Idea from util-linux
	 * chfn/chsh.  --marekm
	 */
	if (!amroot && getdef_bool ("CHSH_AUTH")) {
		passwd_check(pw->pw_name, pw->pw_passwd);
	}

#else				/* !USE_PAM */
	pampw = getpwuid (getuid ()); /* local, no need for xgetpwuid */
	if (NULL == pampw) {
		fprintf (stderr,
		         _("%s: Cannot determine your user name.\n"),
		         Prog);
		exit (E_NOPERM);
	}

	retval = pam_start (Prog, pampw->pw_name, &conv, &pamh);

	if (PAM_SUCCESS == retval) {
		retval = pam_authenticate (pamh, 0);
	}

	if (PAM_SUCCESS == retval) {
		retval = pam_acct_mgmt (pamh, 0);
	}

	if (PAM_SUCCESS != retval) {
		fprintf (stderr, _("%s: PAM: %s\n"),
		         Prog, pam_strerror (pamh, retval));
		SYSLOG(LOG_ERR, "%s", pam_strerror(pamh, retval));
		if (NULL != pamh) {
			(void) pam_end (pamh, retval);
		}
		exit (E_NOPERM);
	}
	(void) pam_end (pamh, retval);
#endif				/* USE_PAM */
}

/*
 * update_shell - update the user's shell in the passwd database
 *
 *	Commit the user's entry after changing her shell field.
 *
 *	It will not return in case of error.
 */
static void update_shell (const char *user, char *newshell, const struct option_flags *flags)
{
	const struct passwd *pw;	/* Password entry from /etc/passwd   */
	struct passwd pwent;		/* New password entry                */
	bool process_selinux;

	process_selinux = !flags->chroot;

	/*
	 * Before going any further, raise the ulimit to prevent
	 * colliding into a lowered ulimit, and set the real UID
	 * to root to protect against unexpected signals. Any
	 * keyboard signals are set to be ignored.
	 */
	if (setuid (0) != 0) {
		SYSLOG(LOG_ERR, "can't setuid(0)");
		fputs (_("Cannot change ID to root.\n"), stderr);
		fail_exit (1, process_selinux);
	}
	pwd_init ();

	/*
	 * The passwd entry is now ready to be committed back to
	 * the password file. Get a lock on the file and open it.
	 */
	if (pw_lock () == 0) {
		fprintf (stderr, _("%s: cannot lock %s; try again later.\n"),
		         Prog, pw_dbname ());
		fail_exit (1, process_selinux);
	}
	pw_locked = true;
	if (pw_open (O_CREAT | O_RDWR) == 0) {
		fprintf (stderr, _("%s: cannot open %s\n"), Prog, pw_dbname ());
		SYSLOG(LOG_WARN, "cannot open %s", pw_dbname());
		fail_exit (1, process_selinux);
	}

	/*
	 * Get the entry to update using pw_locate() - we want the real
	 * one from /etc/passwd, not the one from getpwnam() which could
	 * contain the shadow password if (despite the warnings) someone
	 * enables AUTOSHADOW (or SHADOW_COMPAT in libc).  --marekm
	 */
	pw = pw_locate (user);
	if (NULL == pw) {
		fprintf (stderr,
		         _("%s: user '%s' does not exist in %s\n"),
		         Prog, user, pw_dbname ());
		fail_exit (1, process_selinux);
	}

	/*
	 * Make a copy of the entry, then change the shell field. The other
	 * fields remain unchanged.
	 */
	pwent = *pw;
	pwent.pw_shell = newshell;

	/*
	 * Update the passwd file entry. If there is a DBM file, update
	 * that entry as well.
	 */
	if (pw_update (&pwent) == 0) {
		fprintf (stderr,
		         _("%s: failed to prepare the new %s entry '%s'\n"),
		         Prog, pw_dbname (), pwent.pw_name);
		fail_exit (1, process_selinux);
	}

	/*
	 * Changes have all been made, so commit them and unlock the file.
	 */
	if (pw_close (process_selinux) == 0) {
		fprintf (stderr, _("%s: failure while writing changes to %s\n"), Prog, pw_dbname ());
		SYSLOG(LOG_ERR, "failure while writing changes to %s", pw_dbname());
		fail_exit (1, process_selinux);
	}
	if (pw_unlock (process_selinux) == 0) {
		fprintf (stderr, _("%s: failed to unlock %s\n"), Prog, pw_dbname ());
		SYSLOG(LOG_ERR, "failed to unlock %s", pw_dbname());
		/* continue */
	}
	pw_locked= false;
}

/*
 * chsh - this command controls changes to the user's shell
 *
 *	The only supported option is -s which permits the the login shell to
 *	be set from the command line.
 */
int main (int argc, char **argv)
{
	char *user;		/* User name                         */
	const struct passwd *pw;	/* Password entry from /etc/passwd   */
	struct option_flags  flags = {.chroot = false};
	bool process_selinux;

	sanitize_env ();
	check_fds ();

	log_set_progname(Prog);
	log_set_logfd(stderr);

	(void) setlocale (LC_ALL, "");
	(void) bindtextdomain (PACKAGE, LOCALEDIR);
	(void) textdomain (PACKAGE);

	process_root_flag ("-R", argc, argv);

	/*
	 * This command behaves different for root and non-root users.
	 */
	amroot = (getuid () == 0);

	OPENLOG (Prog);

	/* parse the command line options */
	process_flags (argc, argv, &flags);
	process_selinux = !flags.chroot;

	/*
	 * Get the name of the user to check. It is either the command line
	 * name, or the name getlogin() returns.
	 */
	if (optind < argc) {
		if (!is_valid_user_name (argv[optind])) {
			fprintf (stderr, _("%s: Provided user name is not a valid name\n"), Prog);
			fail_exit (1, process_selinux);
		}
		user = argv[optind];
		pw = xgetpwnam (user);
		if (NULL == pw) {
			fprintf (stderr,
			         _("%s: user '%s' does not exist\n"), Prog, user);
			fail_exit (1, process_selinux);
		}
	} else {
		pw = get_my_pwent ();
		if (NULL == pw) {
			fprintf (stderr,
			         _("%s: Cannot determine your user name.\n"),
			         Prog);
			SYSLOG(LOG_WARN, "Cannot determine the user name of the caller (UID %lu)",
			       (unsigned long) getuid());
			fail_exit (1, process_selinux);
		}
		user = xstrdup (pw->pw_name);
	}

	check_perms (pw, &flags);

	/*
	 * Now get the login shell. Either get it from the password
	 * file, or use the value from the command line.
	 */
	if (!sflg) {
		strtcpy_a(loginsh, pw->pw_shell);
	}

	/*
	 * If the login shell was not set on the command line, let the user
	 * interactively change it.
	 */
	if (!sflg) {
		printf (_("Changing the login shell for %s\n"), user);
		new_fields ();
	}

	/*
	 * Check all of the fields for valid information. The shell
	 * field may not contain any illegal characters. Non-privileged
	 * users are restricted to using the shells in /etc/shells.
	 * The shell must be executable by the user.
	 */
	if (valid_field (loginsh, ":,=\n") != 0) {
		fprintf (stderr, _("%s: Invalid entry: %s\n"), Prog, loginsh);
		fail_exit (1, process_selinux);
	}
	if (!streq(loginsh, "")
	    && (loginsh[0] != '/'
	        || is_restricted_shell (loginsh, process_selinux)
	        || (access (loginsh, X_OK) != 0)))
	{
		if (amroot) {
			fprintf (stderr, _("%s: Warning: %s is an invalid shell\n"), Prog, loginsh);
		} else {
			fprintf (stderr, _("%s: %s is an invalid shell\n"), Prog, loginsh);
			fail_exit (1, process_selinux);
		}
	}

	/* Even for root, warn if an invalid shell is specified. */
	if (!streq(loginsh, "")) {
		/* But not if an empty string is given, documented as meaning the default shell */
		if (access (loginsh, F_OK) != 0) {
			fprintf (stderr, _("%s: Warning: %s does not exist\n"), Prog, loginsh);
		} else if (access (loginsh, X_OK) != 0) {
			fprintf (stderr, _("%s: Warning: %s is not executable\n"), Prog, loginsh);
		}
	}

	update_shell (user, loginsh, &flags);

	SYSLOG(LOG_INFO, "changed user '%s' shell to '%s'", user, loginsh);

	nscd_flush_cache ("passwd");
	sssd_flush_cache (SSSD_DB_PASSWD);

	closelog ();
	exit (E_SUCCESS);
}

