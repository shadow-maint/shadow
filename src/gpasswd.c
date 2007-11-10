/*
 * Copyright 1990 - 1994, Julianne Frances Haugh
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

#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include "defines.h"
#include "exitcodes.h"
#include "groupio.h"
#include "nscd.h"
#include "prototypes.h"
#ifdef	SHADOWGRP
#include "sgroupio.h"
#endif
/*
 * Global variables
 */
static char *Prog;

#ifdef SHADOWGRP
static int is_shadowgrp;
#endif

static int
 aflg = 0, Aflg = 0, dflg = 0, Mflg = 0, rflg = 0, Rflg = 0;

unsigned int bywho = -1;

#ifndef	RETRIES
#define	RETRIES	3
#endif

/* local function prototypes */
static void usage (void);
static RETSIGTYPE catch_signals (int);
static int check_list (const char *);

/*
 * usage - display usage message
 */
static void usage (void)
{
	fprintf (stderr, _("Usage: %s [-r|-R] group\n"), Prog);
	fprintf (stderr, _("       %s [-a user] group\n"), Prog);
	fprintf (stderr, _("       %s [-d user] group\n"), Prog);
#ifdef	SHADOWGRP
	fprintf (stderr,
		 _("       %s [-A user,...] [-M user,...] group\n"), Prog);
#else
	fprintf (stderr, _("       %s [-M user,...] group\n"), Prog);
#endif
	exit (E_USAGE);
}

/*
 * catch_signals - set or reset termio modes.
 *
 *	catch_signals() is called before processing begins. signal() is then
 *	called with catch_signals() as the signal handler. If signal later
 *	calls catch_signals() with a signal number, the terminal modes are
 *	then reset.
 */
static RETSIGTYPE catch_signals (int killed)
{
	static TERMIO sgtty;

	if (killed)
		STTY (0, &sgtty);
	else
		GTTY (0, &sgtty);

	if (killed) {
		putchar ('\n');
		fflush (stdout);
		exit (killed);
	}
}

/*
 * check_list - check a comma-separated list of user names for validity
 *
 *	check_list scans a comma-separated list of user names and checks
 *	that each listed name exists.
 */
static int check_list (const char *users)
{
	const char *start, *end;
	char username[32];
	int errors = 0;
	int len;

	for (start = users; start && *start; start = end) {
		if ((end = strchr (start, ','))) {
			len = end - start;
			end++;
		} else {
			len = strlen (start);
		}

		if (len > sizeof (username) - 1)
			len = sizeof (username) - 1;
		strncpy (username, start, len);
		username[len] = '\0';

		/*
		 * This user must exist.
		 */

		if (!getpwnam (username)) {
			fprintf (stderr, _("%s: unknown user %s\n"),
				 Prog, username);
			errors++;
		}
	}
	return errors;
}

static void failure (void)
{
	fprintf (stderr, _("%s: Permission denied.\n"), Prog);
	exit (1);
}

/*
 * gpasswd - administer the /etc/group file
 *
 *	-a user		add user to the named group
 *	-d user		remove user from the named group
 *	-r		remove password from the named group
 *	-R		restrict access to the named group
 *	-A user,...	make list of users the administrative users
 *	-M user,...	make list of users the group members
 */
int main (int argc, char **argv)
{
	int flag;
	char *cp;
	int amroot;
	int retries;
	struct group *gr = NULL;
	struct group grent;
	static char pass[BUFSIZ];

#ifdef	SHADOWGRP
	struct sgrp *sg = NULL;
	struct sgrp sgent;
	char *admins = NULL;
#endif
	struct passwd *pw = NULL;
	char *myname;
	char *user = NULL;
	char *group = NULL;
	char *members = NULL;

#ifdef WITH_AUDIT
	audit_help_open ();
#endif

	sanitize_env ();
	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);

	/*
	 * Make a note of whether or not this command was invoked by root.
	 * This will be used to bypass certain checks later on. Also, set
	 * the real user ID to match the effective user ID. This will
	 * prevent the invoker from issuing signals which would interfer
	 * with this command.
	 */
	amroot = getuid () == 0;
	bywho = getuid ();
	Prog = Basename (argv[0]);

	OPENLOG ("gpasswd");
	setbuf (stdout, NULL);
	setbuf (stderr, NULL);

#ifdef SHADOWGRP
	is_shadowgrp = sgr_file_present ();
#endif
	while ((flag = getopt (argc, argv, "a:A:d:gM:rR")) != EOF) {
		switch (flag) {
		case 'a':	/* add a user */
			user = optarg;
			if (!getpwnam (user)) {
				fprintf (stderr,
					 _("%s: unknown user %s\n"), Prog,
					 user);
#ifdef WITH_AUDIT
				audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
					      "adding to group", user, -1, 0);
#endif
				exit (1);
			}
			aflg++;
			break;
#ifdef SHADOWGRP
		case 'A':
			if (!amroot) {
#ifdef WITH_AUDIT
				audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
					      "Listing administrators", NULL,
					      bywho, 0);
#endif
				failure ();
			}
			if (!is_shadowgrp) {
				fprintf (stderr,
					 _
					 ("%s: shadow group passwords required for -A\n"),
					 Prog);
				exit (2);
			}
			admins = optarg;
			if (check_list (admins))
				exit (1);
			Aflg++;
			break;
#endif
		case 'd':	/* delete a user */
			dflg++;
			user = optarg;
			break;
		case 'g':	/* no-op from normal password */
			break;
		case 'M':
			if (!amroot) {
#ifdef WITH_AUDIT
				audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
					      "listing members", NULL, bywho,
					      0);
#endif
				failure ();
			}
			members = optarg;
			if (check_list (members))
				exit (1);
			Mflg++;
			break;
		case 'r':	/* remove group password */
			rflg++;
			break;
		case 'R':	/* restrict group password */
			Rflg++;
			break;
		default:
			usage ();
		}
	}

	/*
	 * Make sure exclusive flags are exclusive
	 */

	if (aflg + dflg + rflg + Rflg + (Aflg || Mflg) > 1)
		usage ();

	/*
	 * Determine the name of the user that invoked this command. This
	 * is really hit or miss because there are so many ways that command
	 * can be executed and so many ways to trip up the routines that
	 * report the user name.
	 */

	pw = get_my_pwent ();
	if (!pw) {
		fprintf (stderr, _("Who are you?\n"));
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "user lookup", NULL,
			      bywho, 0);
#endif
		failure ();
	}
	myname = xstrdup (pw->pw_name);

	/*
	 * Get the name of the group that is being affected. The group entry
	 * will be completely replicated so it may be modified later on.
	 */

	/*
	 * XXX - should get the entry using gr_locate() and modify that,
	 * getgrnam() could give us a NIS group.  --marekm
	 */
	if (!(group = argv[optind]))
		usage ();

	if (!(gr = getgrnam (group))) {
		fprintf (stderr, _("unknown group: %s\n"), group);
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "group lookup", group,
			      -1, 0);
#endif
		failure ();
	}
	grent = *gr;
	grent.gr_name = xstrdup (gr->gr_name);
	grent.gr_passwd = xstrdup (gr->gr_passwd);

	grent.gr_mem = dup_list (gr->gr_mem);
#ifdef	SHADOWGRP
	if ((sg = getsgnam (group))) {
		sgent = *sg;
		sgent.sg_name = xstrdup (sg->sg_name);
		sgent.sg_passwd = xstrdup (sg->sg_passwd);

		sgent.sg_mem = dup_list (sg->sg_mem);
		sgent.sg_adm = dup_list (sg->sg_adm);
	} else {
		sgent.sg_name = xstrdup (group);
		sgent.sg_passwd = grent.gr_passwd;
		grent.gr_passwd = "!";	/* XXX warning: const */

		sgent.sg_mem = dup_list (grent.gr_mem);

		sgent.sg_adm = (char **) xmalloc (sizeof (char *) * 2);
#ifdef FIRST_MEMBER_IS_ADMIN
		if (sgent.sg_mem[0]) {
			sgent.sg_adm[0] = xstrdup (sgent.sg_mem[0]);
			sgent.sg_adm[1] = 0;
		} else
#endif
			sgent.sg_adm[0] = 0;

		sg = &sgent;
	}

	/*
	 * The policy here for changing a group is that 1) you must be root
	 * or 2). you must be listed as an administrative member. 
	 * Administrative members can do anything to a group that the root
	 * user can.
	 */
	if (!amroot && !is_on_list (sgent.sg_adm, myname)) {
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "modify group", group,
			      -1, 0);
#endif
		failure ();
	}
#else				/* ! SHADOWGRP */

#ifdef FIRST_MEMBER_IS_ADMIN
	/*
	 * The policy here for changing a group is that 1) you must bes root
	 * or 2) you must be the first listed member of the group. The
	 * first listed member of a group can do anything to that group that
	 * the root user can. The rationale for this hack is that the FIRST
	 * user is probably the most important user in this entire group.
	 */
	if (!amroot) {
		if (grent.gr_mem[0] == (char *) 0) {
#ifdef WITH_AUDIT
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
				      "modifying group", group, -1, 0);
#endif
			failure ();
		}

		if (strcmp (grent.gr_mem[0], myname) != 0) {
#ifdef WITH_AUDIT
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
				      "modifying group", myname, -1, 0);
#endif
			failure ();
		}
	}
#else
	/*
	 * This feature enabled by default could be a security problem when
	 * installed on existing systems where the first group member might
	 * be just a normal user.  --marekm
	 */
	if (!amroot) {
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "modifying group",
			      group, -1, 0);
#endif
		failure ();
	}
#endif

#endif				/* SHADOWGRP */

	/*
	 * Removing a password is straight forward. Just set the password
	 * field to a "".
	 */
	if (rflg) {
		grent.gr_passwd = "";	/* XXX warning: const */
#ifdef	SHADOWGRP
		sgent.sg_passwd = "";	/* XXX warning: const */
#endif
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			      "deleting group password", group, -1, 1);
#endif
		SYSLOG ((LOG_INFO, "remove password from group %s by %s",
			 group, myname));
		goto output;
	} else if (Rflg) {
		/*
		 * Same thing for restricting the group. Set the password
		 * field to "!".
		 */
		grent.gr_passwd = "!";	/* XXX warning: const */
#ifdef	SHADOWGRP
		sgent.sg_passwd = "!";	/* XXX warning: const */
#endif
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			      "restrict access to group", group, -1, 1);
#endif
		SYSLOG ((LOG_INFO, "restrict access to group %s by %s",
			 group, myname));
		goto output;
	}

	/*
	 * Adding a member to a member list is pretty straightforward as
	 * well. Call the appropriate routine and split.
	 */
	if (aflg) {
		printf (_("Adding user %s to group %s\n"), user, group);
		grent.gr_mem = add_list (grent.gr_mem, user);
#ifdef	SHADOWGRP
		sgent.sg_mem = add_list (sgent.sg_mem, user);
#endif
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "adding group member",
			      user, -1, 1);
#endif
		SYSLOG ((LOG_INFO, "add member %s to group %s by %s", user,
			 group, myname));
		goto output;
	}

	/*
	 * Removing a member from the member list is the same deal as adding
	 * one, except the routine is different.
	 */
	if (dflg) {
		int removed = 0;

		printf (_("Removing user %s from group %s\n"), user, group);

		if (is_on_list (grent.gr_mem, user)) {
			removed = 1;
			grent.gr_mem = del_list (grent.gr_mem, user);
		}
#ifdef	SHADOWGRP
		if (is_on_list (sgent.sg_mem, user)) {
			removed = 1;
			sgent.sg_mem = del_list (sgent.sg_mem, user);
		}
#endif
		if (!removed) {
			fprintf (stderr, _("%s: unknown member %s\n"),
				 Prog, user);
#ifdef WITH_AUDIT
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
				      "deleting member", user, -1, 0);
#endif
			exit (1);
		}
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "deleting member",
			      user, -1, 1);
#endif
		SYSLOG ((LOG_INFO, "remove member %s from group %s by %s",
			 user, group, myname));
		goto output;
	}
#ifdef	SHADOWGRP
	/*
	 * Replacing the entire list of administators is simple. Check the
	 * list to make sure everyone is a real user. Then slap the new list
	 * in place.
	 */
	if (Aflg) {
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "setting group admin",
			      group, -1, 1);
#endif
		SYSLOG ((LOG_INFO, "set administrators of %s to %s",
			 group, admins));
		sgent.sg_adm = comma_to_list (admins);
		if (!Mflg)
			goto output;
	}
#endif

	/*
	 * Replacing the entire list of members is simple. Check the list to
	 * make sure everyone is a real user. Then slap the new list in
	 * place.
	 */
	if (Mflg) {
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			      "setting group members", group, -1, 1);
#endif
		SYSLOG ((LOG_INFO, "set members of %s to %s", group, members));
#ifdef SHADOWGRP
		sgent.sg_mem = comma_to_list (members);
#endif
		grent.gr_mem = comma_to_list (members);
		goto output;
	}

	/*
	 * If the password is being changed, the input and output must both
	 * be a tty. The typical keyboard signals are caught so the termio
	 * modes can be restored.
	 */
	if (!isatty (0) || !isatty (1)) {
		fprintf (stderr, _("%s: Not a tty\n"), Prog);
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "changing password",
			      group, -1, 0);
#endif
		exit (1);
	}

	catch_signals (0);	/* save tty modes */

	signal (SIGHUP, catch_signals);
	signal (SIGINT, catch_signals);
	signal (SIGQUIT, catch_signals);
	signal (SIGTERM, catch_signals);
#ifdef	SIGTSTP
	signal (SIGTSTP, catch_signals);
#endif

	/*
	 * A new password is to be entered and it must be encrypted, etc.
	 * The password will be prompted for twice, and both entries must be
	 * identical. There is no need to validate the old password since
	 * the invoker is either the group owner, or root.
	 */
	printf (_("Changing the password for group %s\n"), group);

	for (retries = 0; retries < RETRIES; retries++) {
		if (!(cp = getpass (_("New Password: "))))
			exit (1);

		STRFCPY (pass, cp);
		strzero (cp);
		if (!(cp = getpass (_("Re-enter new password: "))))
			exit (1);

		if (strcmp (pass, cp) == 0) {
			strzero (cp);
			break;
		}

		strzero (cp);
		memzero (pass, sizeof pass);

		if (retries + 1 < RETRIES) {
			puts (_("They don't match; try again"));
#ifdef WITH_AUDIT
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
				      "changing password", group, -1, 0);
#endif
		}
	}

	if (retries == RETRIES) {
		fprintf (stderr, _("%s: Try again later\n"), Prog);
		exit (1);
	}

	cp = pw_encrypt (pass, crypt_make_salt ());
	memzero (pass, sizeof pass);
#ifdef SHADOWGRP
	if (is_shadowgrp)
		sgent.sg_passwd = cp;
	else
#endif
		grent.gr_passwd = cp;
#ifdef WITH_AUDIT
	audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "changing password", group,
		      -1, 1);
#endif
	SYSLOG ((LOG_INFO, "change the password for group %s by %s", group,
		 myname));

	/*
	 * This is the common arrival point to output the new group file.
	 * The freshly crafted entry is in allocated space. The group file
	 * will be locked and opened for writing. The new entry will be
	 * output, etc.
	 */
      output:
	if (setuid (0)) {
		fprintf (stderr, _("Cannot change ID to root.\n"));
		SYSLOG ((LOG_ERR, "can't setuid(0)"));
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "changing id to root",
			      group, -1, 0);
#endif
		closelog ();
		exit (1);
	}
	pwd_init ();

	if (!gr_lock ()) {
		fprintf (stderr, _("%s: can't get lock\n"), Prog);
		SYSLOG ((LOG_WARN, "failed to get lock for /etc/group"));
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "locking /etc/group",
			      group, -1, 0);
#endif
		exit (1);
	}
#ifdef	SHADOWGRP
	if (is_shadowgrp && !sgr_lock ()) {
		fprintf (stderr, _("%s: can't get shadow lock\n"), Prog);
		SYSLOG ((LOG_WARN, "failed to get lock for /etc/gshadow"));
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			      "locking /etc/gshadow", group, -1, 0);
#endif
		exit (1);
	}
#endif
	if (!gr_open (O_RDWR)) {
		fprintf (stderr, _("%s: can't open file\n"), Prog);
		SYSLOG ((LOG_WARN, "cannot open /etc/group"));
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "opening /etc/group",
			      group, -1, 0);
#endif
		exit (1);
	}
#ifdef	SHADOWGRP
	if (is_shadowgrp && !sgr_open (O_RDWR)) {
		fprintf (stderr, _("%s: can't open shadow file\n"), Prog);
		SYSLOG ((LOG_WARN, "cannot open /etc/gshadow"));
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			      "opening /etc/gshadow", group, -1, 0);
#endif
		exit (1);
	}
#endif
	if (!gr_update (&grent)) {
		fprintf (stderr, _("%s: can't update entry\n"), Prog);
		SYSLOG ((LOG_WARN, "cannot update /etc/group"));
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "updating /etc/group",
			      group, -1, 0);
#endif
		exit (1);
	}
#ifdef	SHADOWGRP
	if (is_shadowgrp && !sgr_update (&sgent)) {
		fprintf (stderr, _("%s: can't update shadow entry\n"), Prog);
		SYSLOG ((LOG_WARN, "cannot update /etc/gshadow"));
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			      "updating /etc/gshadow", group, -1, 0);
#endif
		exit (1);
	}
#endif
	if (!gr_close ()) {
		fprintf (stderr, _("%s: can't re-write file\n"), Prog);
		SYSLOG ((LOG_WARN, "cannot re-write /etc/group"));
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			      "rewriting /etc/group", group, -1, 0);
#endif
		exit (1);
	}
#ifdef	SHADOWGRP
	if (is_shadowgrp && !sgr_close ()) {
		fprintf (stderr, _("%s: can't re-write shadow file\n"), Prog);
		SYSLOG ((LOG_WARN, "cannot re-write /etc/gshadow"));
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			      "rewriting /etc/gshadow", group, -1, 0);
#endif
		exit (1);
	}
	if (is_shadowgrp)
		sgr_unlock ();
#endif
	if (!gr_unlock ()) {
		fprintf (stderr, _("%s: can't unlock file\n"), Prog);
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			      "unlocking group file", group, -1, 0);
#endif
		exit (1);
	}

	nscd_flush_cache ("group");

	exit (E_SUCCESS);
}
