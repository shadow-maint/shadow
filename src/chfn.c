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
RCSID(PKG_VER "$Id: chfn.c,v 1.15 1999/07/09 18:02:43 marekm Exp $")

#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include "prototypes.h"
#include "defines.h"

#include <pwd.h>
#include "pwio.h"
#include "getdef.h"
#include "pwauth.h"

#ifdef HAVE_SHADOW_H
#include <shadow.h>
#endif

#ifdef USE_PAM
#include "pam_defs.h"
#endif

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

/*
 * External identifiers
 */

extern	int	optind;
extern	char	*optarg;
#ifdef	NDBM
extern	int	pw_dbm_mode;
#endif

/*
 * #defines for messages.  This facilitates foreign language conversion
 * since all messages are defined right here.
 */

#define WRONGPWD2	"incorrect password for `%s'"
#define	PWDBUSY2	"can't lock /etc/passwd\n"
#define	OPNERROR2	"can't open /etc/passwd\n"
#define	UPDERROR2	"error updating passwd entry\n"
#define	DBMERROR2	"error updating DBM passwd entry.\n"
#define	NOTROOT2	"can't setuid(0).\n"
#define	CLSERROR2	"can't rewrite /etc/passwd.\n"
#define	UNLKERROR2	"can't unlock /etc/passwd.\n"
#define	CHGGECOS	"changed user `%s' information.\n"

/* local function prototypes */
static void usage P_((void));
static int may_change_field P_((int));
static void new_fields P_((void));
static char *copy_field P_((char *, char *, char *));
int main P_((int, char **));

/*
 * usage - print command line syntax and exit
 */

static void
usage(void)
{
	if (amroot)
		fprintf(stderr,
		_("Usage: %s [ -f full_name ] [ -r room_no ] [ -w work_ph ]\n\t[ -h home_ph ] [ -o other ] [ user ]\n"),
		Prog);
	else
		fprintf(stderr,
		_("Usage: %s [ -f full_name ] [ -r room_no ] [ -w work_ph ] [ -h home_ph ]\n"),
		Prog);
	exit(1);
}


static int
may_change_field(int field)
{
	const char *cp;

	/*
	 * CHFN_RESTRICT can now specify exactly which fields may be
	 * changed by regular users, by using any combination of the
	 * following letters:
	 *  f - full name
	 *  r - room number
	 *  w - work phone
	 *  h - home phone
	 *
	 * This makes it possible to disallow changing the room number
	 * information, for example - this feature was suggested by
	 * Maciej 'Tycoon' Majchrowski.
	 *
	 * For backward compatibility, "yes" is equivalent to "rwh",
	 * "no" is equivalent to "frwh".  Only root can change anything
	 * if the string is empty or not defined at all.
	 */
	if (amroot)
		return 1;
	cp = getdef_str("CHFN_RESTRICT");
	if (!cp)
		cp = "";
	else if (strcmp(cp, "yes") == 0)
		cp = "rwh";
	else if (strcmp(cp, "no") == 0)
		cp = "frwh";
	if (strchr(cp, field))
		return 1;
	return 0;
}

/*
 * new_fields - change the user's GECOS information interactively
 *
 * prompt the user for each of the four fields and fill in the fields
 * from the user's response, or leave alone if nothing was entered.
 */

static void
new_fields(void)
{
	printf(_("Enter the new value, or press return for the default\n"));

	if (may_change_field('f'))
		change_field(fullnm, sizeof fullnm, _("Full Name"));
	else
		printf(_("\tFull Name: %s\n"), fullnm);

	if (may_change_field('r'))
		change_field(roomno, sizeof roomno, _("Room Number"));
	else
		printf(_("\tRoom Number: %s\n"), roomno);

	if (may_change_field('w'))
		change_field(workph, sizeof workph, _("Work Phone"));
	else
		printf(_("\tWork Phone: %s\n"), workph);

	if (may_change_field('h'))
		change_field(homeph, sizeof homeph, _("Home Phone"));
	else
		printf(_("\tHome Phone: %s\n"), homeph);

	if (amroot)
		change_field(slop, sizeof slop, _("Other"));
}

/*
 * copy_field - get the next field from the gecos field
 *
 * copy_field copies the next field from the gecos field, returning a
 * pointer to the field which follows, or NULL if there are no more
 * fields.
 *
 *	in - the current GECOS field
 *	out - where to copy the field to
 *	extra - fields with '=' get copied here
 */

static char *
copy_field(char *in, char *out, char *extra)
{
	char *cp = NULL;

	while (in) {
		if ((cp = strchr (in, ',')))
			*cp++ = '\0';

		if (! strchr (in, '='))
			break;

		if (extra) {
			if (extra[0])
				strcat (extra, ",");

			strcat (extra, in);
		}
		in = cp;
	}
	if (in && out)
		strcpy (out, in);

	return cp;
}


/*
 * chfn - change a user's password file information
 *
 *	This command controls the GECOS field information in the
 *	password file entry.
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

int
main(int argc, char **argv)
{
	char	*cp;			/* temporary character pointer       */
	const struct passwd *pw;	/* password file entry               */
	struct	passwd	pwent;		/* modified password file entry      */
	char	old_gecos[BUFSIZ];	/* buffer for old GECOS fields       */
	char	new_gecos[BUFSIZ];	/* buffer for new GECOS fields       */
	int	flag;			/* flag currently being processed    */
	int	fflg = 0;		/* -f - set full name                */
	int	rflg = 0;		/* -r - set room number              */
	int	wflg = 0;		/* -w - set work phone number        */
	int	hflg = 0;		/* -h - set home phone number        */
	int	oflg = 0;		/* -o - set other information        */
	char *user;

	sanitize_env();
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	/*
	 * This command behaves different for root and non-root
	 * users.
	 */

	amroot = (getuid () == 0);
#ifdef	NDBM
	pw_dbm_mode = O_RDWR;
#endif

	/*
	 * Get the program name.  The program name is used as a
	 * prefix to most error messages.  It is also used as input
	 * to the openlog() function for error logging.
	 */

	Prog = Basename(argv[0]);

	openlog("chfn", LOG_PID, LOG_AUTH);

	/* 
	 * The remaining arguments will be processed one by one and
	 * executed by this command.  The name is the last argument
	 * if it does not begin with a "-", otherwise the name is
	 * determined from the environment and must agree with the
	 * real UID.  Also, the UID will be checked for any commands
	 * which are restricted to root only.
	 */

	while ((flag = getopt (argc, argv, "f:r:w:h:o:")) != EOF) {
		switch (flag) {
			case 'f':
				if (!may_change_field('f')) {
					fprintf(stderr, _("%s: Permission denied.\n"), Prog);
					exit(1);
				}
				fflg++;
				STRFCPY(fullnm, optarg);
				break;
			case 'r':
				if (!may_change_field('r')) {
					fprintf(stderr, _("%s: Permission denied.\n"), Prog);
					exit(1);
				}
				rflg++;
				STRFCPY(roomno, optarg);
				break;
			case 'w':
				if (!may_change_field('w')) {
					fprintf(stderr, _("%s: Permission denied.\n"), Prog);
					exit(1);
				}
				wflg++;
				STRFCPY(workph, optarg);
				break;
			case 'h':
				if (!may_change_field('h')) {
					fprintf(stderr, _("%s: Permission denied.\n"), Prog);
					exit(1);
				}
				hflg++;
				STRFCPY(homeph, optarg);
				break;
			case 'o':
				if (!amroot) {
					fprintf(stderr, _("%s: Permission denied.\n"), Prog);
					exit(1);
				}
				oflg++;
				STRFCPY(slop, optarg);
				break;
			default:
				usage();
		}
	}

	/*
	 * Get the name of the user to check.  It is either
	 * the command line name, or the name getlogin()
	 * returns.
	 */

	if (optind < argc) {
		user = argv[optind];
		pw = getpwnam(user);
		if (!pw) {
			fprintf(stderr, _("%s: Unknown user %s\n"), Prog, user);
			exit(1);
		}
	} else {
		pw = get_my_pwent();
		if (!pw) {
			fprintf(stderr, _("%s: Cannot determine your user name.\n"), Prog);
			exit(1);
		}
		user = xstrdup(pw->pw_name);
	}

#ifdef	USE_NIS
	/*
	 * Now we make sure this is a LOCAL password entry for
	 * this user ...
	 */

	if (__ispwNIS ()) {
		char	*nis_domain;
		char	*nis_master;

		fprintf (stderr, _("%s: cannot change user `%s' on NIS client.\n"), Prog, user);

		if (! yp_get_default_domain (&nis_domain) &&
				! yp_master (nis_domain, "passwd.byname",
				&nis_master)) {
			fprintf (stderr, _("%s: `%s' is the NIS master for this client.\n"), Prog, nis_master);
		}
		exit (1);
	}
#endif

	/*
	 * Non-privileged users are only allowed to change the
	 * gecos field if the UID of the user matches the current
	 * real UID.
	 */

	if (!amroot && pw->pw_uid != getuid()) {
		fprintf (stderr, _("%s: Permission denied.\n"), Prog);
		closelog();
		exit(1);
	}

	/*
	 * Non-privileged users are optionally authenticated
	 * (must enter the password of the user whose information
	 * is being changed) before any changes can be made.
	 * Idea from util-linux chfn/chsh.  --marekm
	 */

	if (!amroot && getdef_bool("CHFN_AUTH"))
		passwd_check(pw->pw_name, pw->pw_passwd, "chfn");
	
	/*
	 * Now get the full name.  It is the first comma separated field
	 * in the GECOS field.
	 */

	STRFCPY(old_gecos, pw->pw_gecos);
	cp = copy_field (old_gecos, fflg ? (char *) 0:fullnm, slop);

	/*
	 * Now get the room number.  It is the next comma separated field,
	 * if there is indeed one.
	 */

	if (cp)
		cp = copy_field (cp, rflg ? (char *) 0:roomno, slop);

	/*
	 * Now get the work phone number.  It is the third field.
	 */

	if (cp)
		cp = copy_field (cp, wflg ? (char *) 0:workph, slop);

	/*
	 * Now get the home phone number.  It is the fourth field.
	 */

	if (cp)
		cp = copy_field (cp, hflg ? (char *) 0:homeph, slop);

	/*
	 * Anything left over is "slop".
	 */

	if (cp && !oflg) {
		if (slop[0])
			strcat (slop, ",");

		strcat (slop, cp);
	}

	/*
	 * If none of the fields were changed from the command line,
	 * let the user interactively change them.
	 */

	if (!fflg && !rflg && !wflg && !hflg && !oflg) {
		printf(_("Changing the user information for %s\n"), user);
		new_fields();
	}

	/*
	 * Check all of the fields for valid information
	 */

	if (valid_field(fullnm, ":,=")) {
		fprintf(stderr, _("%s: invalid name: \"%s\"\n"), Prog, fullnm);
		closelog();
		exit(1);
	}
	if (valid_field(roomno, ":,=")) {
		fprintf(stderr, _("%s: invalid room number: \"%s\"\n"), Prog, roomno);
		closelog();
		exit(1);
	}
	if (valid_field(workph, ":,=")) {
		fprintf(stderr, _("%s: invalid work phone: \"%s\"\n"), Prog, workph);
		closelog();
		exit(1);
	}
	if (valid_field (homeph, ":,=")) {
		fprintf(stderr, _("%s: invalid home phone: \"%s\"\n"), Prog, homeph);
		closelog();
		exit(1);
	}
	if (valid_field(slop, ":")) {
		fprintf(stderr, _("%s: \"%s\" contains illegal characters\n"), Prog, slop);
		closelog();
		exit(1);
	}

	/*
	 * Build the new GECOS field by plastering all the pieces together,
	 * if they will fit ...
	 */

	if (strlen(fullnm) + strlen(roomno) + strlen(workph) +
			strlen(homeph) + strlen(slop) > (unsigned int) 80) {
		fprintf(stderr, _("%s: fields too long\n"), Prog);
		closelog();
		exit(1);
	}
	snprintf(new_gecos, sizeof new_gecos, "%s,%s,%s,%s%s%s",
		 fullnm, roomno, workph, homeph, slop[0] ? "," : "", slop);

	/*
	 * Before going any further, raise the ulimit to prevent
	 * colliding into a lowered ulimit, and set the real UID
	 * to root to protect against unexpected signals.  Any
	 * keyboard signals are set to be ignored.
	 */

	if (setuid(0)) {
		fprintf(stderr, _("Cannot change ID to root.\n"));
		SYSLOG((LOG_ERR, NOTROOT2));
		closelog();
		exit(1);
	}
	pwd_init();

	/*
	 * The passwd entry is now ready to be committed back to
	 * the password file.  Get a lock on the file and open it.
	 */

	if (!pw_lock()) {
		fprintf(stderr, _("Cannot lock the password file; try again later.\n"));
		SYSLOG((LOG_WARN, PWDBUSY2));
		closelog();
		exit(1);
	}
	if (!pw_open(O_RDWR)) {
		fprintf(stderr, _("Cannot open the password file.\n"));
		pw_unlock();
		SYSLOG((LOG_ERR, OPNERROR2));
		closelog();
		exit(1);
	}

	/*
	 * Get the entry to update using pw_locate() - we want the real
	 * one from /etc/passwd, not the one from getpwnam() which could
	 * contain the shadow password if (despite the warnings) someone
	 * enables AUTOSHADOW (or SHADOW_COMPAT in libc).  --marekm
	 */
	pw = pw_locate(user);
	if (!pw) {
		pw_unlock();
		fprintf(stderr,
			_("%s: %s not found in /etc/passwd\n"), Prog, user);
		exit(1);
	}

	/*
	 * Make a copy of the entry, then change the gecos field.  The other
	 * fields remain unchanged.
	 */
	pwent = *pw;
	pwent.pw_gecos = new_gecos;

	/*
	 * Update the passwd file entry.  If there is a DBM file,
	 * update that entry as well.
	 */

	if (!pw_update(&pwent)) {
		fprintf(stderr, _("Error updating the password entry.\n"));
		pw_unlock();
		SYSLOG((LOG_ERR, UPDERROR2));
		closelog();
		exit(1);
	}
#ifdef NDBM
	if (pw_dbm_present() && !pw_dbm_update(&pwent)) {
		fprintf(stderr, _("Error updating the DBM password entry.\n"));
		pw_unlock ();
		SYSLOG((LOG_ERR, DBMERROR2));
		closelog();
		exit(1);
	}
	endpwent();
#endif

	/*
	 * Changes have all been made, so commit them and unlock the
	 * file.
	 */

	if (!pw_close()) {
		fprintf(stderr, _("Cannot commit password file changes.\n"));
		pw_unlock();
		SYSLOG((LOG_ERR, CLSERROR2));
		closelog();
		exit(1);
	}
	if (!pw_unlock()) {
		fprintf(stderr, _("Cannot unlock the password file.\n"));
		SYSLOG((LOG_ERR, UNLKERROR2));
		closelog();
		exit(1);
	}
	SYSLOG((LOG_INFO, CHGGECOS, user));
	closelog();
	exit (0);
}
