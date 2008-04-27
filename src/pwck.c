/*
 * Copyright (c) 1992 - 1994, Julianne Frances Haugh
 * Copyright (c) 1996 - 2000, Marek Michałkiewicz
 * Copyright (c) 2001       , Michał Moskal
 * Copyright (c) 2001 - 2006, Tomasz Kłoczko
 * Copyright (c) 2007 - 2008, Nicolas François
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

#include <config.h>

#ident "$Id$"

#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include "chkname.h"
#include "commonio.h"
#include "defines.h"
#include "prototypes.h"
#include "pwio.h"
#include "shadowio.h"
#include "getdef.h"
#include "nscd.h"

/*
 * Exit codes
 */

#define	E_OKAY		0
#define	E_USAGE		1
#define	E_BADENTRY	2
#define	E_CANTOPEN	3
#define	E_CANTLOCK	4
#define	E_CANTUPDATE	5

/*
 * Global variables
 */

static char *Prog;
static const char *pwd_file = PASSWD_FILE;
static int use_system_pw_file = 1;
static const char *spw_file = SHADOW_FILE;
static int use_system_spw_file = 1;

static int is_shadow = 0;

/* Options */
static int read_only = 0;
static int sort_mode = 0;
static int quiet = 0;		/* don't report warnings, only errors */

/* local function prototypes */
static void usage (void);
static void process_flags (int argc, char **argv);
static void open_files (void);
static void close_files (int changed);
static void check_pw_file (int *errors, int *changed);
static void check_spw_file (int *errors, int *changed);

/*
 * usage - print syntax message and exit
 */
static void usage (void)
{
	fprintf (stderr, _("Usage: %s [-q] [-r] [-s] [passwd [shadow]]\n"),
	         Prog);
	exit (E_USAGE);
}

/*
 * process_flags - parse the command line options
 *
 *	It will not return if an error is encountered.
 */
static void process_flags (int argc, char **argv)
{
	int arg;

	/*
	 * Parse the command line arguments
	 */
	while ((arg = getopt (argc, argv, "eqrs")) != EOF) {
		switch (arg) {
		case 'e':	/* added for Debian shadow-961025-2 compatibility */
		case 'q':
			quiet = 1;
			break;
		case 'r':
			read_only = 1;
			break;
		case 's':
			sort_mode = 1;
			break;
		default:
			usage ();
		}
	}

	if (sort_mode && read_only) {
		fprintf (stderr, _("%s: -s and -r are incompatibile\n"), Prog);
		exit (E_USAGE);
	}

	/*
	 * Make certain we have the right number of arguments
	 */
	if ((argc < optind) || (argc > (optind + 2))) {
		usage ();
	}

	/*
	 * If there are two left over filenames, use those as the password
	 * and shadow password filenames.
	 */
	if (optind != argc) {
		pwd_file = argv[optind];
		pw_name (pwd_file);
		use_system_pw_file = 0;
	}
	if ((optind + 2) == argc) {
		spw_file = argv[optind + 1];
		spw_name (spw_file);
		is_shadow = 1;
		use_system_spw_file = 0;
	} else if (optind == argc) {
		is_shadow = spw_file_present ();
	}
}

/*
 * open_files - open the shadow database
 *
 *	In read-only mode, the databases are not locked and are opened
 *	only for reading.
 */
static void open_files (void)
{
	/*
	 * Lock the files if we aren't in "read-only" mode
	 */
	if (!read_only) {
		if (pw_lock () == 0) {
			fprintf (stderr, _("%s: cannot lock file %s\n"),
			         Prog, pwd_file);
			if (use_system_pw_file) {
				SYSLOG ((LOG_WARN, "cannot lock %s", pwd_file));
			}
			closelog ();
			exit (E_CANTLOCK);
		}
		if (is_shadow && (spw_lock () == 0)) {
			fprintf (stderr, _("%s: cannot lock file %s\n"),
			         Prog, spw_file);
			if (use_system_spw_file) {
				SYSLOG ((LOG_WARN, "cannot lock %s", spw_file));
			}
			closelog ();
			exit (E_CANTLOCK);
		}
	}

	/*
	 * Open the files. Use O_RDONLY if we are in read_only mode, O_RDWR
	 * otherwise.
	 */
	if (pw_open (read_only ? O_RDONLY : O_RDWR) == 0) {
		fprintf (stderr, _("%s: cannot open file %s\n"),
		         Prog, pwd_file);
		if (use_system_pw_file) {
			SYSLOG ((LOG_WARN, "cannot open %s", pwd_file));
		}
		closelog ();
		exit (E_CANTOPEN);
	}
	if (is_shadow && (spw_open (read_only ? O_RDONLY : O_RDWR) == 0)) {
		fprintf (stderr, _("%s: cannot open file %s\n"),
		         Prog, spw_file);
		if (use_system_spw_file) {
			SYSLOG ((LOG_WARN, "cannot open %s", spw_file));
		}
		closelog ();
		exit (E_CANTOPEN);
	}
}

/*
 * close_files - close and unlock the password/shadow databases
 *
 *	If changed is not set, the databases are not closed, and no
 *	changes are committed in the databases. The databases are
 *	unlocked anyway.
 */
static void close_files (int changed)
{
	/*
	 * All done. If there were no change we can just abandon any
	 * changes to the files.
	 */
	if (changed) {
		if (pw_close () == 0) {
			fprintf (stderr, _("%s: cannot update file %s\n"),
			         Prog, pwd_file);
			SYSLOG ((LOG_WARN, "cannot update %s", pwd_file));
			closelog ();
			exit (E_CANTUPDATE);
		}
		if (is_shadow && (spw_close () == 0)) {
			fprintf (stderr, _("%s: cannot update file %s\n"),
			         Prog, spw_file);
			SYSLOG ((LOG_WARN, "cannot update %s", spw_file));
			closelog ();
			exit (E_CANTUPDATE);
		}
	}

	/*
	 * Don't be anti-social - unlock the files when you're done.
	 */
	if (is_shadow) {
		spw_unlock ();
	}
	(void) pw_unlock ();
}

/*
 * check_pw_file - check the content of the passwd file
 */
static void check_pw_file (int *errors, int *changed)
{
	struct commonio_entry *pfe, *tpfe;
	struct passwd *pwd;
	struct spwd *spw;

	/*
	 * Loop through the entire password file.
	 */
	for (pfe = __pw_get_head (); NULL != pfe; pfe = pfe->next) {
		/*
		 * If this is a NIS line, skip it. You can't "know" what NIS
		 * is going to do without directly asking NIS ...
		 */
		if ((pfe->line[0] == '+') || (pfe->line[0] == '-')) {
			continue;
		}

		/*
		 * Start with the entries that are completely corrupt.  They
		 * have no (struct passwd) entry because they couldn't be
		 * parsed properly.
		 */
		if (NULL == pfe->eptr) {
			/*
			 * Tell the user this entire line is bogus and ask
			 * them to delete it.
			 */
			puts (_("invalid password file entry"));
			printf (_("delete line '%s'? "), pfe->line);
			*errors += 1;

			/*
			 * prompt the user to delete the entry or not
			 */
			if (!yes_or_no (read_only)) {
				continue;
			}

			/*
			 * All password file deletions wind up here. This
			 * code removes the current entry from the linked
			 * list. When done, it skips back to the top of the
			 * loop to try out the next list element.
			 */
		      delete_pw:
			SYSLOG ((LOG_INFO, "delete passwd line `%s'",
			         pfe->line));
			*changed = 1;

			__pw_del_entry (pfe);
			continue;
		}

		/*
		 * Password structure is good, start using it.
		 */
		pwd = pfe->eptr;

		/*
		 * Make sure this entry has a unique name.
		 */
		for (tpfe = __pw_get_head (); NULL != tpfe; tpfe = tpfe->next) {
			const struct passwd *ent = tpfe->eptr;

			/*
			 * Don't check this entry
			 */
			if (tpfe == pfe) {
				continue;
			}

			/*
			 * Don't check invalid entries.
			 */
			if (NULL == ent) {
				continue;
			}

			if (strcmp (pwd->pw_name, ent->pw_name) != 0) {
				continue;
			}

			/*
			 * Tell the user this entry is a duplicate of
			 * another and ask them to delete it.
			 */
			puts (_("duplicate password entry"));
			printf (_("delete line '%s'? "), pfe->line);
			*errors += 1;

			/*
			 * prompt the user to delete the entry or not
			 */
			if (yes_or_no (read_only)) {
				goto delete_pw;
			}
		}

		/*
		 * Check for invalid usernames.  --marekm
		 */
		if (!check_user_name (pwd->pw_name)) {
			printf (_("invalid user name '%s'\n"), pwd->pw_name);
			*errors += 1;
		}

		/*
		 * Make sure the primary group exists
		 */
		/* local, no need for xgetgrgid */
		if (!quiet && (NULL == getgrgid (pwd->pw_gid))) {

			/*
			 * No primary group, just give a warning
			 */

			printf (_("user %s: no group %u\n"),
			        pwd->pw_name, pwd->pw_gid);
			*errors += 1;
		}

		/*
		 * Make sure the home directory exists
		 */
		if (!quiet && (access (pwd->pw_dir, F_OK) != 0)) {
			/*
			 * Home directory doesn't exist, give a warning
			 */
			printf (_("user %s: directory %s does not exist\n"),
			        pwd->pw_name, pwd->pw_dir);
			*errors += 1;
		}

		/*
		 * Make sure the login shell is executable
		 */
		if (   !quiet
		    && ('\0' != pwd->pw_shell[0])
		    && (access (pwd->pw_shell, F_OK) != 0)) {

			/*
			 * Login shell doesn't exist, give a warning
			 */
			printf (_("user %s: program %s does not exist\n"),
			        pwd->pw_name, pwd->pw_shell);
			*errors += 1;
		}

		/*
		 * Make sure this entry exists in the /etc/shadow file.
		 */

		if (is_shadow) {
			spw = (struct spwd *) spw_locate (pwd->pw_name);
			if (spw == NULL) {
				printf (_("no matching password file entry in %s\n"),
				        spw_file);
				printf (_("add user '%s' in %s? "),
				        pwd->pw_name, spw_file);
				*errors += 1;
				if (yes_or_no (read_only)) {
					struct spwd sp;
					struct passwd pw;

					sp.sp_namp = pwd->pw_name;
					sp.sp_pwdp = pwd->pw_passwd;
					sp.sp_min =
					    getdef_num ("PASS_MIN_DAYS", -1);
					sp.sp_max =
					    getdef_num ("PASS_MAX_DAYS", -1);
					sp.sp_warn =
					    getdef_num ("PASS_WARN_AGE", -1);
					sp.sp_inact = -1;
					sp.sp_expire = -1;
					sp.sp_flag = -1;
					sp.sp_lstchg =
					    time ((time_t *) 0) / (24L * 3600L);
					*changed = 1;

					if (spw_update (&sp) == 0) {
						fprintf (stderr,
						         _("%s: can't update shadow entry for %s\n"),
						         Prog, sp.sp_namp);
						exit (E_CANTUPDATE);
					}
					/* remove password from /etc/passwd */
					pw = *pwd;
					pw.pw_passwd = SHADOW_PASSWD_STRING;	/* XXX warning: const */
					if (pw_update (&pw) == 0) {
						fprintf (stderr,
						         _("%s: can't update passwd entry for %s\n"),
						         Prog, pw.pw_name);
						exit (E_CANTUPDATE);
					}
				}
			}
		}
	}
}

/*
 * check_spw_file - check the content of the shadowed password file (shadow)
 */
static void check_spw_file (int *errors, int *changed)
{
	struct commonio_entry *spe, *tspe;
	struct spwd *spw;

	/*
	 * Loop through the entire shadow password file.
	 */
	for (spe = __spw_get_head (); NULL != spe; spe = spe->next) {
		/*
		 * Do not treat lines which were missing in shadow
		 * and were added earlier.
		 */
		if (spe->line == NULL) {
			continue;
		}

		/*
		 * If this is a NIS line, skip it. You can't "know" what NIS
		 * is going to do without directly asking NIS ...
		 */
		if ((spe->line[0] == '+') || (spe->line[0] == '-')) {
			continue;
		}

		/*
		 * Start with the entries that are completely corrupt. They
		 * have no (struct spwd) entry because they couldn't be
		 * parsed properly.
		 */
		if (NULL == spe->eptr) {
			/*
			 * Tell the user this entire line is bogus and ask
			 * them to delete it.
			 */
			puts (_("invalid shadow password file entry"));
			printf (_("delete line '%s'? "), spe->line);
			*errors += 1;

			/*
			 * prompt the user to delete the entry or not
			 */
			if (!yes_or_no (read_only)) {
				continue;
			}

			/*
			 * All shadow file deletions wind up here. This code
			 * removes the current entry from the linked list.
			 * When done, it skips back to the top of the loop
			 * to try out the next list element.
			 */
		      delete_spw:
			SYSLOG ((LOG_INFO, "delete shadow line `%s'",
			         spe->line));
			*changed = 1;

			__spw_del_entry (spe);
			continue;
		}

		/*
		 * Shadow password structure is good, start using it.
		 */
		spw = spe->eptr;

		/*
		 * Make sure this entry has a unique name.
		 */
		for (tspe = __spw_get_head (); NULL != tspe; tspe = tspe->next) {
			const struct spwd *ent = tspe->eptr;

			/*
			 * Don't check this entry
			 */
			if (tspe == spe) {
				continue;
			}

			/*
			 * Don't check invalid entries.
			 */
			if (NULL == ent) {
				continue;
			}

			if (strcmp (spw->sp_namp, ent->sp_namp) != 0) {
				continue;
			}

			/*
			 * Tell the user this entry is a duplicate of
			 * another and ask them to delete it.
			 */
			puts (_("duplicate shadow password entry"));
			printf (_("delete line '%s'? "), spe->line);
			*errors += 1;

			/*
			 * prompt the user to delete the entry or not
			 */
			if (yes_or_no (read_only)) {
				goto delete_spw;
			}
		}

		/*
		 * Make sure this entry exists in the /etc/passwd
		 * file.
		 */
		if (pw_locate (spw->sp_namp) == NULL) {
			/*
			 * Tell the user this entry has no matching
			 * /etc/passwd entry and ask them to delete it.
			 */
			printf (_("no matching password file entry in %s\n"),
			        pwd_file);
			printf (_("delete line '%s'? "), spe->line);
			*errors += 1;

			/*
			 * prompt the user to delete the entry or not
			 */
			if (yes_or_no (read_only)) {
				goto delete_spw;
			}
		}

		/*
		 * Warn if last password change in the future.  --marekm
		 */
		if (!quiet
		    && (spw->sp_lstchg > time ((time_t *) 0) / SCALE)) {
			printf (_("user %s: last password change in the future\n"),
			        spw->sp_namp);
			*errors += 1;
		}
	}
}

/*
 * pwck - verify password file integrity
 */
int main (int argc, char **argv)
{
	int errors = 0;
	int changed = 0;

	/*
	 * Get my name so that I can use it to report errors.
	 */
	Prog = Basename (argv[0]);

	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);

	OPENLOG ("pwck");

	/* Parse the command line arguments */
	process_flags (argc, argv);

	open_files ();

	if (sort_mode) {
		pw_sort ();
		if (is_shadow) {
			spw_sort ();
		}
		changed = 1;
	} else {
		check_pw_file (&errors, &changed);

		if (is_shadow) {
			check_spw_file (&errors, &changed);
		}
	}

	close_files (changed);

	nscd_flush_cache ("passwd");

	/*
	 * Tell the user what we did and exit.
	 */
	if (errors != 0) {
		printf (changed ?
		        _("%s: the files have been updated\n") :
		        _("%s: no changes\n"), Prog);
	}

	closelog ();
	exit (errors ? E_BADENTRY : E_OKAY);
}

