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

#ident "$Id$"

#include <fcntl.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#ifdef WITH_SELINUX
#include <selinux/selinux.h>
#include <selinux/av_permissions.h>
#endif
#include "defines.h"
#include "exitcodes.h"
#include "getdef.h"
#include "nscd.h"
#ifdef USE_PAM
#include "pam_defs.h"
#endif
#include "prototypes.h"
#include "pwauth.h"
#include "pwio.h"
/*
 * Global variables.
 */
static char *Prog;
static char fullnm[BUFSIZ];
static char roomno[BUFSIZ];
static char workph[BUFSIZ];
static char homeph[BUFSIZ];
static char slop[BUFSIZ];
static int amroot;
/* Flags */
static int fflg = 0;		/* -f - set full name                */
static int rflg = 0;		/* -r - set room number              */
static int wflg = 0;		/* -w - set work phone number        */
static int hflg = 0;		/* -h - set home phone number        */
static int oflg = 0;		/* -o - set other information        */
#ifdef USE_PAM
static pam_handle_t *pamh = NULL;
#endif

/*
 * External identifiers
 */

/* local function prototypes */
static void usage (void);
static int may_change_field (int);
static void new_fields (void);
static char *copy_field (char *, char *, char *);
static void process_flags (int argc, char **argv);
static void check_perms (const struct passwd *pw);
static void update_gecos (const char *user, char *gecos);
static void get_old_fields (const char *gecos);

/*
 * usage - print command line syntax and exit
 */
static void usage (void)
{
	if (amroot) {
		fprintf (stderr,
		         _("Usage: %s [-f full_name] [-r room_no] "
		           "[-w work_ph]\n"
		           "\t[-h home_ph] [-o other] [user]\n"), Prog);
	} else {
		fprintf (stderr,
		         _("Usage: %s [-f full_name] [-r room_no] "
		           "[-w work_ph] [-h home_ph]\n"), Prog);
	}
	exit (E_USAGE);
}

/*
 * may_change_field - indicate if the user is allowed to change a given field
 *                    of her gecos information
 *
 *	root can change any field.
 *
 *	field should be one of 'f', 'r', 'w', 'h'
 *
 *	Return 1 if the user can change the field and 0 otherwise.
 */
static int may_change_field (int field)
{
	const char *cp;

	/*
	 * CHFN_RESTRICT can now specify exactly which fields may be changed
	 * by regular users, by using any combination of the following
	 * letters:
	 *  f - full name
	 *  r - room number
	 *  w - work phone
	 *  h - home phone
	 *
	 * This makes it possible to disallow changing the room number
	 * information, for example - this feature was suggested by Maciej
	 * 'Tycoon' Majchrowski.
	 *
	 * For backward compatibility, "yes" is equivalent to "rwh",
	 * "no" is equivalent to "frwh". Only root can change anything
	 * if the string is empty or not defined at all.
	 */
	if (amroot) {
		return 1;
	}

	cp = getdef_str ("CHFN_RESTRICT");
	if (NULL == cp) {
		cp = "";
	} else if (strcmp (cp, "yes") == 0) {
		cp = "rwh";
	} else if (strcmp (cp, "no") == 0) {
		cp = "frwh";
	}

	if (strchr (cp, field) != NULL) {
		return 1;
	}

	return 0;
}

/*
 * new_fields - change the user's GECOS information interactively
 *
 * prompt the user for each of the four fields and fill in the fields from
 * the user's response, or leave alone if nothing was entered.
 */
static void new_fields (void)
{
	puts (_("Enter the new value, or press ENTER for the default\n"));

	if (may_change_field ('f')) {
		change_field (fullnm, sizeof fullnm, _("Full Name"));
	} else {
		printf (_("\tFull Name: %s\n"), fullnm);
	}

	if (may_change_field ('r')) {
		change_field (roomno, sizeof roomno, _("Room Number"));
	} else {
		printf (_("\tRoom Number: %s\n"), roomno);
	}

	if (may_change_field ('w')) {
		change_field (workph, sizeof workph, _("Work Phone"));
	} else {
		printf (_("\tWork Phone: %s\n"), workph);
	}

	if (may_change_field ('h')) {
		change_field (homeph, sizeof homeph, _("Home Phone"));
	} else {
		printf (_("\tHome Phone: %s\n"), homeph);
	}

	if (amroot) {
		change_field (slop, sizeof slop, _("Other"));
	}
}

/*
 * copy_field - get the next field from the gecos field
 *
 * copy_field copies the next field from the gecos field, returning a
 * pointer to the field which follows, or NULL if there are no more fields.
 *
 *	in - the current GECOS field
 *	out - where to copy the field to
 *	extra - fields with '=' get copied here
 */
static char *copy_field (char *in, char *out, char *extra)
{
	char *cp = NULL;

	while (NULL != in) {
		cp = strchr (in, ',');
		if (NULL != cp) {
			*cp++ = '\0';
		}

		if (strchr (in, '=') == NULL) {
			break;
		}

		if (NULL != extra) {
			if ('\0' != extra[0]) {
				strcat (extra, ",");
			}

			strcat (extra, in);
		}
		in = cp;
	}
	if ((NULL != in) && (NULL != out)) {
		strcpy (out, in);
	}

	return cp;
}

/*
 * process_flags - parse the command line options
 *
 *	It will not return if an error is encountered.
 */
static void process_flags (int argc, char **argv)
{
	int flag;		/* flag currently being processed    */

	/* 
	 * The remaining arguments will be processed one by one and executed
	 * by this command. The name is the last argument if it does not
	 * begin with a "-", otherwise the name is determined from the
	 * environment and must agree with the real UID. Also, the UID will
	 * be checked for any commands which are restricted to root only.
	 */
	while ((flag = getopt (argc, argv, "f:r:w:h:o:")) != EOF) {
		switch (flag) {
		case 'f':
			if (!may_change_field ('f')) {
				fprintf (stderr,
				         _("%s: Permission denied.\n"), Prog);
				exit (E_NOPERM);
			}
			fflg++;
			STRFCPY (fullnm, optarg);
			break;
		case 'h':
			if (!may_change_field ('h')) {
				fprintf (stderr,
				         _("%s: Permission denied.\n"), Prog);
				exit (E_NOPERM);
			}
			hflg++;
			STRFCPY (homeph, optarg);
			break;
		case 'r':
			if (!may_change_field ('r')) {
				fprintf (stderr,
				         _("%s: Permission denied.\n"), Prog);
				exit (E_NOPERM);
			}
			rflg++;
			STRFCPY (roomno, optarg);
			break;
		case 'o':
			if (!amroot) {
				fprintf (stderr,
				         _("%s: Permission denied.\n"), Prog);
				exit (E_NOPERM);
			}
			oflg++;
			STRFCPY (slop, optarg);
			break;
		case 'w':
			if (!may_change_field ('w')) {
				fprintf (stderr,
				         _("%s: Permission denied.\n"), Prog);
				exit (E_NOPERM);
			}
			wflg++;
			STRFCPY (workph, optarg);
			break;
		default:
			usage ();
		}
	}
}

/*
 * check_perms - check if the caller is allowed to add a group
 *
 *	Non-root users are only allowed to change their gecos field.
 *	(see also may_change_field())
 *
 *	Non-root users must be authenticated.
 *
 *	It will not return if the user is not allowed.
 */
static void check_perms (const struct passwd *pw)
{
#ifdef USE_PAM
	int retval;
	struct passwd *pampw;
#endif

	/*
	 * Non-privileged users are only allowed to change the gecos field
	 * if the UID of the user matches the current real UID.
	 */
	if (!amroot && pw->pw_uid != getuid ()) {
		fprintf (stderr, _("%s: Permission denied.\n"), Prog);
		closelog ();
		exit (E_NOPERM);
	}
#ifdef WITH_SELINUX
	/*
	 * If the UID of the user does not match the current real UID,
	 * check if the change is allowed by SELinux policy.
	 */
	if ((pw->pw_uid != getuid ())
	    && (is_selinux_enabled () > 0)
	    && (selinux_check_passwd_access (PASSWD__CHFN) != 0)) {
		fprintf (stderr, _("%s: Permission denied.\n"), Prog);
		closelog ();
		exit (E_NOPERM);
	}
#endif

#ifndef USE_PAM
	/*
	 * Non-privileged users are optionally authenticated (must enter the
	 * password of the user whose information is being changed) before
	 * any changes can be made. Idea from util-linux chfn/chsh. 
	 * --marekm
	 */
	if (!amroot && getdef_bool ("CHFN_AUTH")) {
		passwd_check (pw->pw_name, pw->pw_passwd, "chfn");
	}

#else				/* !USE_PAM */
	retval = PAM_SUCCESS;

	pampw = getpwuid (getuid ()); /* local, no need for xgetpwuid */
	if (pampw == NULL) {
		retval = PAM_USER_UNKNOWN;
	}

	if (retval == PAM_SUCCESS) {
		retval = pam_start ("chfn", pampw->pw_name, &conv, &pamh);
	}

	if (retval == PAM_SUCCESS) {
		retval = pam_authenticate (pamh, 0);
		if (retval != PAM_SUCCESS) {
			pam_end (pamh, retval);
		}
	}

	if (retval == PAM_SUCCESS) {
		retval = pam_acct_mgmt (pamh, 0);
		if (retval != PAM_SUCCESS) {
			pam_end (pamh, retval);
		}
	}

	if (retval != PAM_SUCCESS) {
		fprintf (stderr, _("%s: PAM authentication failed\n"), Prog);
		exit (E_NOPERM);
	}
#endif				/* USE_PAM */
}

/*
 * update_gecos - update the gecos fields in the password database
 *
 *	Commit the user's entry after changing her gecos field.
 */
static void update_gecos (const char *user, char *gecos)
{
	const struct passwd *pw;	/* The user's password file entry */
	struct passwd pwent;		/* modified password file entry */

	/*
	 * Before going any further, raise the ulimit to prevent colliding
	 * into a lowered ulimit, and set the real UID to root to protect
	 * against unexpected signals. Any keyboard signals are set to be
	 * ignored.
	 */
	if (setuid (0) != 0) {
		fputs (_("Cannot change ID to root.\n"), stderr);
		SYSLOG ((LOG_ERR, "can't setuid(0)"));
		closelog ();
		exit (E_NOPERM);
	}
	pwd_init ();

	/*
	 * The passwd entry is now ready to be committed back to the
	 * password file. Get a lock on the file and open it.
	 */
	if (pw_lock () == 0) {
		fputs (_("Cannot lock the password file; try again later.\n"),
		       stderr);
		SYSLOG ((LOG_WARN, "can't lock /etc/passwd"));
		closelog ();
		exit (E_NOPERM);
	}
	if (pw_open (O_RDWR) == 0) {
		fputs (_("Cannot open the password file.\n"), stderr);
		pw_unlock ();
		SYSLOG ((LOG_ERR, "can't open /etc/passwd"));
		closelog ();
		exit (E_NOPERM);
	}

	/*
	 * Get the entry to update using pw_locate() - we want the real one
	 * from /etc/passwd, not the one from getpwnam() which could contain
	 * the shadow password if (despite the warnings) someone enables
	 * AUTOSHADOW (or SHADOW_COMPAT in libc).  --marekm
	 */
	pw = pw_locate (user);
	if (NULL == pw) {
		pw_unlock ();
		fprintf (stderr,
		         _("%s: %s not found in /etc/passwd\n"), Prog, user);
		exit (E_NOPERM);
	}

	/*
	 * Make a copy of the entry, then change the gecos field. The other
	 * fields remain unchanged.
	 */
	pwent = *pw;
	pwent.pw_gecos = gecos;

	/*
	 * Update the passwd file entry. If there is a DBM file, update that
	 * entry as well.
	 */
	if (pw_update (&pwent) == 0) {
		fputs (_("Error updating the password entry.\n"), stderr);
		pw_unlock ();
		SYSLOG ((LOG_ERR, "error updating passwd entry"));
		closelog ();
		exit (E_NOPERM);
	}

	/*
	 * Changes have all been made, so commit them and unlock the file.
	 */
	if (pw_close () == 0) {
		fputs (_("Cannot commit password file changes.\n"), stderr);
		pw_unlock ();
		SYSLOG ((LOG_ERR, "can't rewrite /etc/passwd"));
		closelog ();
		exit (E_NOPERM);
	}
	if (pw_unlock () == 0) {
		fputs (_("Cannot unlock the password file.\n"), stderr);
		SYSLOG ((LOG_ERR, "can't unlock /etc/passwd"));
		closelog ();
		exit (E_NOPERM);
	}
}

/*
 * get_old_fields - parse the old gecos and use the old value for the fields
 *                  which are not set on the command line
 */
static void get_old_fields (const char *gecos)
{
	char *cp;		/* temporary character pointer       */
	char old_gecos[BUFSIZ];	/* buffer for old GECOS fields       */
	STRFCPY (old_gecos, gecos);

	/*
	 * Now get the full name. It is the first comma separated field in
	 * the GECOS field.
	 */
	cp = copy_field (old_gecos, fflg ? (char *) 0 : fullnm, slop);

	/*
	 * Now get the room number. It is the next comma separated field,
	 * if there is indeed one.
	 */
	if (NULL != cp) {
		cp = copy_field (cp, rflg ? (char *) 0 : roomno, slop);
	}

	/*
	 * Now get the work phone number. It is the third field.
	 */
	if (NULL != cp) {
		cp = copy_field (cp, wflg ? (char *) 0 : workph, slop);
	}

	/*
	 * Now get the home phone number. It is the fourth field.
	 */
	if (NULL != cp) {
		cp = copy_field (cp, hflg ? (char *) 0 : homeph, slop);
	}

	/*
	 * Anything left over is "slop".
	 */
	if ((NULL != cp) && !oflg) {
		if ('\0' != slop[0]) {
			strcat (slop, ",");
		}

		strcat (slop, cp);
	}
}

/*
 * check_fields - check all of the fields for valid information
 *
 *	It will not return if a field is not valid.
 */
static void check_fields (void)
{
	if (valid_field (fullnm, ":,=")) {
		fprintf (stderr, _("%s: invalid name: '%s'\n"), Prog, fullnm);
		closelog ();
		exit (E_NOPERM);
	}
	if (valid_field (roomno, ":,=")) {
		fprintf (stderr, _("%s: invalid room number: '%s'\n"),
		         Prog, roomno);
		closelog ();
		exit (E_NOPERM);
	}
	if (valid_field (workph, ":,=")) {
		fprintf (stderr, _("%s: invalid work phone: '%s'\n"),
		         Prog, workph);
		closelog ();
		exit (E_NOPERM);
	}
	if (valid_field (homeph, ":,=")) {
		fprintf (stderr, _("%s: invalid home phone: '%s'\n"),
		         Prog, homeph);
		closelog ();
		exit (E_NOPERM);
	}
	if (valid_field (slop, ":")) {
		fprintf (stderr,
		         _("%s: '%s' contains illegal characters\n"),
		         Prog, slop);
		closelog ();
		exit (E_NOPERM);
	}
}

/*
 * chfn - change a user's password file information
 *
 *	This command controls the GECOS field information in the password
 *	file entry.
 *
 *	The valid options are
 *
 *	-f	full name
 *	-r	room number
 *	-w	work phone number
 *	-h	home phone number
 *	-o	other information (*)
 *
 *	(*) requires root permission to execute.
 */
int main (int argc, char **argv)
{
	const struct passwd *pw;	/* password file entry               */
	char new_gecos[BUFSIZ];	/* buffer for new GECOS fields       */
	char *user;

	sanitize_env ();
	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);

	/*
	 * This command behaves different for root and non-root
	 * users.
	 */
	amroot = (getuid () == 0);

	/*
	 * Get the program name. The program name is used as a
	 * prefix to most error messages.
	 */
	Prog = Basename (argv[0]);

	OPENLOG ("chfn");

	/* parse the command line options */
	process_flags (argc, argv);

	/*
	 * Get the name of the user to check. It is either the command line
	 * name, or the name getlogin() returns.
	 */
	if (optind < argc) {
		user = argv[optind];
		pw = xgetpwnam (user);
		if (NULL == pw) {
			fprintf (stderr, _("%s: unknown user %s\n"), Prog,
			         user);
			exit (E_NOPERM);
		}
	} else {
		pw = get_my_pwent ();
		if (NULL == pw) {
			fprintf (stderr,
			         _
			         ("%s: Cannot determine your user name.\n"),
			         Prog);
			exit (E_NOPERM);
		}
		user = xstrdup (pw->pw_name);
	}

#ifdef	USE_NIS
	/*
	 * Now we make sure this is a LOCAL password entry for this user ...
	 */
	if (__ispwNIS ()) {
		char *nis_domain;
		char *nis_master;

		fprintf (stderr,
		         _("%s: cannot change user '%s' on NIS client.\n"),
		         Prog, user);

		if (!yp_get_default_domain (&nis_domain) &&
		    !yp_master (nis_domain, "passwd.byname", &nis_master)) {
			fprintf (stderr,
			         _
			         ("%s: '%s' is the NIS master for this client.\n"),
			         Prog, nis_master);
		}
		exit (E_NOPERM);
	}
#endif

	/* Check that the caller is allowed to change the gecos of the
	 * specified user */
	check_perms (pw);

	/* If some fields were not set on the command line, load the value from
	 * the old gecos fields. */
	get_old_fields (pw->pw_gecos);

	/*
	 * If none of the fields were changed from the command line, let the
	 * user interactively change them.
	 */
	if (!fflg && !rflg && !wflg && !hflg && !oflg) {
		printf (_("Changing the user information for %s\n"), user);
		new_fields ();
	}

	/*
	 * Check all of the fields for valid information
	 */
	check_fields ();

	/*
	 * Build the new GECOS field by plastering all the pieces together,
	 * if they will fit ...
	 */
	if ((strlen (fullnm) + strlen (roomno) + strlen (workph) +
	     strlen (homeph) + strlen (slop)) > (unsigned int) 80) {
		fprintf (stderr, _("%s: fields too long\n"), Prog);
		closelog ();
		exit (E_NOPERM);
	}
	snprintf (new_gecos, sizeof new_gecos, "%s,%s,%s,%s%s%s",
		  fullnm, roomno, workph, homeph, slop[0] ? "," : "", slop);

	/* Rewrite the user's gecos in the passwd file */
	update_gecos (user, new_gecos);

	SYSLOG ((LOG_INFO, "changed user `%s' information", user));

	nscd_flush_cache ("passwd");

#ifdef USE_PAM
	pam_end (pamh, PAM_SUCCESS);
#endif				/* USE_PAM */

	closelog ();
	exit (E_SUCCESS);
}

