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
#ifdef SHADOWGRP
#include "sgroupio.h"
#endif
/*
 * Global variables
 */
/* The name of this command, as it is invoked */
static char *Prog;

#ifdef SHADOWGRP
/* Indicate if shadow groups are enabled on the system
 * (/etc/gshadow present) */
static int is_shadowgrp;
#endif

/* Flags set by options */
static int
 aflg = 0, Aflg = 0, dflg = 0, Mflg = 0, rflg = 0, Rflg = 0;
/* The name of the group that is being affected */
static char *group = NULL;
/* The name of the user being added (-a) or removed (-d) from group */
static char *user = NULL;
/* The new list of members set with -M */
static char *members = NULL;
#ifdef SHADOWGRP
/* The new list of group administrators set with -A */
static char *admins = NULL;
#endif
/* The name of the caller */
static char *myname = NULL;
/* The UID of the caller */
static unsigned long bywho = -1;
/* Indicate if gpasswd was called by root */
#define amroot	(0 == bywho)

/* The number of retries for th user to provide and repeat a new password */
#ifndef	RETRIES
#define	RETRIES	3
#endif

/* local function prototypes */
static void usage (void);
static RETSIGTYPE catch_signals (int killed);
static int check_list (const char *users);
static void process_flags (int argc, char **argv);
static void check_flags (int argc, int opt_index);
static void open_files (void);
static void close_files (void);
#ifdef SHADOWGRP
static void get_group (struct group *gr, struct sgrp *sg);
static void check_perms (const struct sgrp *sg);
static void update_group (struct group *gr, struct sgrp *sg);
static void change_passwd (struct group *gr, struct sgrp *sg);
#else
static void get_group (struct group *gr);
static void check_perms (const struct group *gr);
static void update_group (struct group *gr);
static void change_passwd (struct group *gr);
#endif

/*
 * usage - display usage message
 */
static void usage (void)
{
	fprintf (stderr, _("Usage: %s [-r|-R] group\n"), Prog);
	fprintf (stderr, _("       %s [-a user] group\n"), Prog);
	fprintf (stderr, _("       %s [-d user] group\n"), Prog);
#ifdef SHADOWGRP
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

	if (killed) {
		STTY (0, &sgtty);
	} else {
		GTTY (0, &sgtty);
	}

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
 *
 *	It returns 0 on success.
 */
static int check_list (const char *users)
{
	const char *start, *end;
	char username[32];
	int errors = 0;
	size_t len;

	for (start = users; start && *start; start = end) {
		end = strchr (start, ',');
		if (NULL != end) {
			len = end - start;
			end++;
		} else {
			len = strlen (start);
		}

		if (len > sizeof (username) - 1) {
			len = sizeof (username) - 1;
		}
		strncpy (username, start, len);
		username[len] = '\0';

		/*
		 * This user must exist.
		 */

		if (!getpwnam (username)) { /* local, no need for xgetpwnam */
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
 * process_flags - process the command line options and arguments
 */
static void process_flags (int argc, char **argv)
{
	int flag;

	while ((flag = getopt (argc, argv, "a:A:d:gM:rR")) != EOF) {
		switch (flag) {
		case 'a':	/* add a user */
			user = optarg;
			/* local, no need for xgetpwnam */
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
			if (check_list (admins)) {
				exit (1);
			}
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
			if (check_list (members)) {
				exit (1);
			}
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

	/* Get the name of the group that is being affected. */
	group = argv[optind];

	check_flags (argc, optind);
}

/*
 * check_flags - check the validity of options
 */
static void check_flags (int argc, int opt_index)
{
	/*
	 * Make sure exclusive flags are exclusive
	 */
	if (aflg + dflg + rflg + Rflg + (Aflg || Mflg) > 1) {
		usage ();
	}

	/*
	 * Make sure one (and only one) group was provided
	 */
	if ((argc != (opt_index+1)) || (NULL == group)) {
		usage ();
	}
}

/*
 * open_files - lock and open the group databases
 *
 *	It will call exit in case of error.
 */
static void open_files (void)
{
	if (gr_lock () == 0) {
		fprintf (stderr, _("%s: can't get lock\n"), Prog);
		SYSLOG ((LOG_WARN, "failed to get lock for /etc/group"));
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
		              "locking /etc/group", group, -1, 0);
#endif
		exit (1);
	}
#ifdef SHADOWGRP
	if (is_shadowgrp && (sgr_lock () == 0)) {
		fprintf (stderr, _("%s: can't get shadow lock\n"), Prog);
		SYSLOG ((LOG_WARN, "failed to get lock for /etc/gshadow"));
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
		              "locking /etc/gshadow", group, -1, 0);
#endif
		exit (1);
	}
#endif
	if (gr_open (O_RDWR) == 0) {
		fprintf (stderr, _("%s: can't open file\n"), Prog);
		SYSLOG ((LOG_WARN, "cannot open /etc/group"));
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
		              "opening /etc/group", group, -1, 0);
#endif
		exit (1);
	}
#ifdef SHADOWGRP
	if (is_shadowgrp && (sgr_open (O_RDWR) == 0)) {
		fprintf (stderr, _("%s: can't open shadow file\n"), Prog);
		SYSLOG ((LOG_WARN, "cannot open /etc/gshadow"));
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
		              "opening /etc/gshadow", group, -1, 0);
#endif
		exit (1);
	}
#endif
}

/*
 * close_files - close and unlock the group databases
 *
 *	This cause any changes in the databases to be committed.
 *
 *	It will call exit in case of error.
 */
static void close_files (void)
{
	if (gr_close () == 0) {
		fprintf (stderr, _("%s: can't re-write file\n"), Prog);
		SYSLOG ((LOG_WARN, "cannot re-write /etc/group"));
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
		              "rewriting /etc/group", group, -1, 0);
#endif
		exit (1);
	}
#ifdef SHADOWGRP
	if (is_shadowgrp && (sgr_close () == 0)) {
		fprintf (stderr, _("%s: can't re-write shadow file\n"), Prog);
		SYSLOG ((LOG_WARN, "cannot re-write /etc/gshadow"));
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
		              "rewriting /etc/gshadow", group, -1, 0);
#endif
		exit (1);
	}
	if (is_shadowgrp) {
		/* TODO: same logging as in open_files & for /etc/group */
		sgr_unlock ();
	}
#endif
	if (gr_unlock () == 0) {
		fprintf (stderr, _("%s: can't unlock file\n"), Prog);
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
		              "unlocking group file", group, -1, 0);
#endif
		exit (1);
	}
}

/*
 * check_perms - check if the user is allowed to change the password of
 *               the specified group.
 *
 *	It only returns if the user is allowed.
 */
#ifdef SHADOWGRP
static void check_perms (const struct sgrp *sg)
#else
static void check_perms (const struct group *gr)
#endif
{
#ifdef SHADOWGRP
	/*
	 * The policy here for changing a group is that 1) you must be root
	 * or 2). you must be listed as an administrative member.
	 * Administrative members can do anything to a group that the root
	 * user can.
	 */
	if (!amroot && !is_on_list (sg->sg_adm, myname)) {
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
		              "modify group", group, -1, 0);
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
		if (gr->gr_mem[0] == (char *) 0) {
#ifdef WITH_AUDIT
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			              "modifying group", group, -1, 0);
#endif
			failure ();
		}

		if (strcmp (gr->gr_mem[0], myname) != 0) {
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
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
		              "modifying group", group, -1, 0);
#endif
		failure ();
	}
#endif
#endif				/* SHADOWGRP */
}

/*
 * update_group - Update the group information in the databases
 */
#ifdef SHADOWGRP
static void update_group (struct group *gr, struct sgrp *sg)
#else
static void update_group (struct group *gr)
#endif
{
	if (!gr_update (gr)) {
		fprintf (stderr, _("%s: can't update entry\n"), Prog);
		SYSLOG ((LOG_WARN, "cannot update /etc/group"));
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
		              "updating /etc/group", group, -1, 0);
#endif
		exit (1);
	}
#ifdef SHADOWGRP
	if (is_shadowgrp && !sgr_update (sg)) {
		fprintf (stderr, _("%s: can't update shadow entry\n"), Prog);
		SYSLOG ((LOG_WARN, "cannot update /etc/gshadow"));
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
		              "updating /etc/gshadow", group, -1, 0);
#endif
		exit (1);
	}
#endif
}

/*
 * get_group - get the current information for the group
 *
 *	The information are copied in group structure(s) so that they can be
 *	modified later.
 */
#ifdef SHADOWGRP
static void get_group (struct group *gr, struct sgrp *sg)
#else
static void get_group (struct group *gr)
#endif
{
	struct group const*tmpgr = NULL;
	struct sgrp const*tmpsg = NULL;

	if (!gr_open (O_RDONLY)) {
		fprintf (stderr, _("%s: can't open file\n"), Prog);
		SYSLOG ((LOG_WARN, "cannot open /etc/group"));
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
		              "opening /etc/group", group, -1, 0);
#endif
		exit (1);
	}

	tmpgr = gr_locate (group);
	if (NULL == tmpgr) {
		fprintf (stderr, _("unknown group: %s\n"), group);
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
		              "group lookup", group, -1, 0);
#endif
		failure ();
	}

	*gr = *tmpgr;
	gr->gr_name = xstrdup (tmpgr->gr_name);
	gr->gr_passwd = xstrdup (tmpgr->gr_passwd);
	gr->gr_mem = dup_list (tmpgr->gr_mem);

	if (!gr_close ()) {
		fprintf (stderr, _("%s: can't close file\n"), Prog);
		SYSLOG ((LOG_WARN, "cannot close /etc/group"));
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
		              "closing /etc/group", group, -1, 0);
#endif
		exit (1);
	}

#ifdef SHADOWGRP
	if (!sgr_open (O_RDONLY)) {
		fprintf (stderr, _("%s: can't open shadow file\n"), Prog);
		SYSLOG ((LOG_WARN, "cannot open /etc/gshadow"));
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
		              "opening /etc/gshadow", group, -1, 0);
#endif
		exit (1);
	}
	tmpsg = sgr_locate (group);
	if (NULL != tmpsg) {
		*sg = *tmpsg;
		sg->sg_name = xstrdup (tmpsg->sg_name);
		sg->sg_passwd = xstrdup (tmpsg->sg_passwd);

		sg->sg_mem = dup_list (tmpsg->sg_mem);
		sg->sg_adm = dup_list (tmpsg->sg_adm);
	} else {
		sg->sg_name = xstrdup (group);
		sg->sg_passwd = gr->gr_passwd;
		gr->gr_passwd = "!";	/* XXX warning: const */

		sg->sg_mem = dup_list (gr->gr_mem);

		sg->sg_adm = (char **) xmalloc (sizeof (char *) * 2);
#ifdef FIRST_MEMBER_IS_ADMIN
		if (sg->sg_mem[0]) {
			sg->sg_adm[0] = xstrdup (sg->sg_mem[0]);
			sg->sg_adm[1] = 0;
		} else
#endif
		{
			sg->sg_adm[0] = 0;
		}

	}
	if (!sgr_close ()) {
		fprintf (stderr, _("%s: can't close shadow file\n"), Prog);
		SYSLOG ((LOG_WARN, "cannot close /etc/gshadow"));
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
		              "closing /etc/gshadow", group, -1, 0);
#endif
		exit (1);
	}
#endif				/* SHADOWGRP */
}

/*
 * change_passwd - change the group's password
 *
 *	Get the new password from the user and update the password in the
 *	group's structure.
 *
 *	It will call exit in case of error.
 */
#ifdef SHADOWGRP
static void change_passwd (struct group *gr, struct sgrp *sg)
#else
static void change_passwd (struct group *gr)
#endif
{
	char *cp;
	static char pass[BUFSIZ];
	int retries;

	/*
	 * A new password is to be entered and it must be encrypted, etc.
	 * The password will be prompted for twice, and both entries must be
	 * identical. There is no need to validate the old password since
	 * the invoker is either the group owner, or root.
	 */
	printf (_("Changing the password for group %s\n"), group);

	for (retries = 0; retries < RETRIES; retries++) {
		cp = getpass (_("New Password: "));
		if (NULL == cp) {
			exit (1);
		}

		STRFCPY (pass, cp);
		strzero (cp);
		cp = getpass (_("Re-enter new password: "));
		if (NULL == cp) {
			exit (1);
		}

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

	cp = pw_encrypt (pass, crypt_make_salt (NULL, NULL));
	memzero (pass, sizeof pass);
#ifdef SHADOWGRP
	if (is_shadowgrp) {
		sg->sg_passwd = cp;
	} else
#endif
	{
		gr->gr_passwd = cp;
	}
#ifdef WITH_AUDIT
	audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
	              "changing password", group, -1, 1);
#endif
	SYSLOG ((LOG_INFO, "change the password for group %s by %s", group,
	        myname));
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
	struct group grent;
#ifdef SHADOWGRP
	struct sgrp sgent;
#endif
	struct passwd *pw = NULL;

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
	bywho = getuid ();
	Prog = Basename (argv[0]);

	OPENLOG ("gpasswd");
	setbuf (stdout, NULL);
	setbuf (stderr, NULL);

#ifdef SHADOWGRP
	is_shadowgrp = sgr_file_present ();
#endif

	/* Parse the options */
	process_flags (argc, argv);

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
	 * Replicate the group so it can be modified later on.
	 */
#ifdef SHADOWGRP
	get_group (&grent, &sgent);
#else
	get_group (&grent);
#endif

	/*
	 * Check if the user is allowed to change the password of this group.
	 */
#ifdef SHADOWGRP
	check_perms (&sgent);
#else
	check_perms (&grent);
#endif

	/*
	 * Removing a password is straight forward. Just set the password
	 * field to a "".
	 */
	if (rflg) {
		grent.gr_passwd = "";	/* XXX warning: const */
#ifdef SHADOWGRP
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
#ifdef SHADOWGRP
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
#ifdef SHADOWGRP
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
#ifdef SHADOWGRP
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
#ifdef SHADOWGRP
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
		if (!Mflg) {
			goto output;
		}
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
#ifdef SIGTSTP
	signal (SIGTSTP, catch_signals);
#endif

	/* Prompt for the new password */
#ifdef SHADOWGRP
	change_passwd (&grent, &sgent);
#else
	change_passwd (&grent);
#endif

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

	open_files ();

#ifdef SHADOWGRP
	update_group (&grent, &sgent);
#else
	update_group (&grent);
#endif

	close_files ();

	nscd_flush_cache ("group");

	exit (E_SUCCESS);
}
