/*
 * Copyright 1992 - 1994, Julianne Frances Haugh
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

#include "rcsid.h"
RCSID (PKG_VER "$Id: pwck.c,v 1.27 2005/05/25 18:20:25 kloczek Exp $")
#include <stdio.h>
#include <fcntl.h>
#include <grp.h>
#include "prototypes.h"
#include "defines.h"
#include "chkname.h"
#include <pwd.h>
#include "commonio.h"
#include "pwio.h"
extern void __pw_del_entry (const struct commonio_entry *);
extern struct commonio_entry *__pw_get_head (void);

#include "shadowio.h"
extern void __spw_del_entry (const struct commonio_entry *);
extern struct commonio_entry *__spw_get_head (void);

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
 * Local variables
 */

static char *Prog;
static const char *pwd_file = PASSWD_FILE;

static const char *spw_file = SHADOW_FILE;
static int read_only = 0;
static int quiet = 0;		/* don't report warnings, only errors */

/* local function prototypes */
static void usage (void);
static int yes_or_no (void);

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
 * yes_or_no - get answer to question from the user
 */

static int yes_or_no (void)
{
	char buf[80];

	/*
	 * In read-only mode all questions are answered "no".
	 */

	if (read_only) {
		puts (_("No"));
		return 0;
	}

	/*
	 * Get a line and see what the first character is.
	 */

	if (fgets (buf, sizeof buf, stdin))
		return buf[0] == 'y' || buf[0] == 'Y';

	return 0;
}

/*
 * pwck - verify password file integrity
 */

int main (int argc, char **argv)
{
	int arg;
	int errors = 0;
	int deleted = 0;
	struct commonio_entry *pfe, *tpfe;
	struct passwd *pwd;
	int sort_mode = 0;

	struct commonio_entry *spe, *tspe;
	struct spwd *spw;
	int is_shadow = 0;

	/*
	 * Get my name so that I can use it to report errors.
	 */

	Prog = Basename (argv[0]);

	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);

	OPENLOG ("pwsk");

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

	if (optind != argc && optind + 1 != argc && optind + 2 != argc)
		usage ();

	/*
	 * If there are two left over filenames, use those as the password
	 * and shadow password filenames.
	 */

	if (optind != argc) {
		pwd_file = argv[optind];
		pw_name (pwd_file);
	}
	if (optind + 2 == argc) {
		spw_file = argv[optind + 1];
		spw_name (spw_file);
		is_shadow = 1;
	} else if (optind == argc)
		is_shadow = spw_file_present ();

	/*
	 * Lock the files if we aren't in "read-only" mode
	 */

	if (!read_only) {
		if (!pw_lock ()) {
			fprintf (stderr, _("%s: cannot lock file %s\n"),
				 Prog, pwd_file);
			if (optind == argc)
				SYSLOG ((LOG_WARN, "cannot lock %s", pwd_file));
			closelog ();
			exit (E_CANTLOCK);
		}
		if (is_shadow && !spw_lock ()) {
			fprintf (stderr, _("%s: cannot lock file %s\n"),
				 Prog, spw_file);
			if (optind == argc)
				SYSLOG ((LOG_WARN, "cannot lock %s", spw_file));
			closelog ();
			exit (E_CANTLOCK);
		}
	}

	/*
	 * Open the files. Use O_RDONLY if we are in read_only mode, O_RDWR
	 * otherwise.
	 */

	if (!pw_open (read_only ? O_RDONLY : O_RDWR)) {
		fprintf (stderr, _("%s: cannot open file %s\n"),
			 Prog, pwd_file);
		if (optind == argc)
			SYSLOG ((LOG_WARN, "cannot open %s", pwd_file));
		closelog ();
		exit (E_CANTOPEN);
	}
	if (is_shadow && !spw_open (read_only ? O_RDONLY : O_RDWR)) {
		fprintf (stderr, _("%s: cannot open file %s\n"),
			 Prog, spw_file);
		if (optind == argc)
			SYSLOG ((LOG_WARN, "cannot open %s", spw_file));
		closelog ();
		exit (E_CANTOPEN);
	}

	if (sort_mode) {
		pw_sort ();
		if (is_shadow)
			spw_sort ();
		goto write_and_bye;
	}

	/*
	 * Loop through the entire password file.
	 */

	for (pfe = __pw_get_head (); pfe; pfe = pfe->next) {
		/*
		 * If this is a NIS line, skip it. You can't "know" what NIS
		 * is going to do without directly asking NIS ...
		 */

		if (pfe->line[0] == '+' || pfe->line[0] == '-')
			continue;

		/*
		 * Start with the entries that are completely corrupt.  They
		 * have no (struct passwd) entry because they couldn't be
		 * parsed properly.
		 */

		if (!pfe->eptr) {

			/*
			 * Tell the user this entire line is bogus and ask
			 * them to delete it.
			 */

			printf (_("invalid password file entry\n"));
			printf (_("delete line `%s'? "), pfe->line);
			errors++;

			/*
			 * prompt the user to delete the entry or not
			 */

			if (!yes_or_no ())
				continue;

			/*
			 * All password file deletions wind up here. This
			 * code removes the current entry from the linked
			 * list. When done, it skips back to the top of the
			 * loop to try out the next list element.
			 */

		      delete_pw:
			SYSLOG ((LOG_INFO, "delete passwd line `%s'",
				 pfe->line));
			deleted++;

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

		for (tpfe = __pw_get_head (); tpfe; tpfe = tpfe->next) {
			const struct passwd *ent = tpfe->eptr;

			/*
			 * Don't check this entry
			 */

			if (tpfe == pfe)
				continue;

			/*
			 * Don't check invalid entries.
			 */

			if (!ent)
				continue;

			if (strcmp (pwd->pw_name, ent->pw_name) != 0)
				continue;

			/*
			 * Tell the user this entry is a duplicate of
			 * another and ask them to delete it.
			 */

			puts (_("duplicate password entry\n"));
			printf (_("delete line `%s'? "), pfe->line);
			errors++;

			/*
			 * prompt the user to delete the entry or not
			 */

			if (yes_or_no ())
				goto delete_pw;
		}

		/*
		 * Check for invalid usernames.  --marekm
		 */
		if (!check_user_name (pwd->pw_name)) {
			printf (_("invalid user name '%s'\n"), pwd->pw_name);
			errors++;
		}

		/*
		 * Make sure the primary group exists
		 */

		if (!quiet && !getgrgid (pwd->pw_gid)) {

			/*
			 * No primary group, just give a warning
			 */

			printf (_("user %s: no group %u\n"),
				pwd->pw_name, pwd->pw_gid);
			errors++;
		}

		/*
		 * Make sure the home directory exists
		 */

		if (!quiet && access (pwd->pw_dir, F_OK)) {

			/*
			 * Home directory doesn't exist, give a warning
			 */

			printf (_
				("user %s: directory %s does not exist\n"),
				pwd->pw_name, pwd->pw_dir);
			errors++;
		}

		/*
		 * Make sure the login shell is executable
		 */

		if (!quiet && pwd->pw_shell[0]
		    && access (pwd->pw_shell, F_OK)) {

			/*
			 * Login shell doesn't exist, give a warning
			 */

			printf (_("user %s: program %s does not exist\n"),
				pwd->pw_name, pwd->pw_shell);
			errors++;
		}
	}

	if (!is_shadow)
		goto shadow_done;

	/*
	 * Loop through the entire shadow password file.
	 */

	for (spe = __spw_get_head (); spe; spe = spe->next) {
		/*
		 * If this is a NIS line, skip it. You can't "know" what NIS
		 * is going to do without directly asking NIS ...
		 */

		if (spe->line[0] == '+' || spe->line[0] == '-')
			continue;

		/*
		 * Start with the entries that are completely corrupt. They
		 * have no (struct spwd) entry because they couldn't be
		 * parsed properly.
		 */

		if (!spe->eptr) {

			/*
			 * Tell the user this entire line is bogus and ask
			 * them to delete it.
			 */

			printf (_("invalid shadow password file entry\n"));
			printf (_("delete line `%s'? "), spe->line);
			errors++;

			/*
			 * prompt the user to delete the entry or not
			 */

			if (!yes_or_no ())
				continue;

			/*
			 * All shadow file deletions wind up here. This code
			 * removes the current entry from the linked list.
			 * When done, it skips back to the top of the loop
			 * to try out the next list element.
			 */

		      delete_spw:
			SYSLOG ((LOG_INFO, "delete shadow line `%s'",
				 spe->line));
			deleted++;

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

		for (tspe = __spw_get_head (); tspe; tspe = tspe->next) {
			const struct spwd *ent = tspe->eptr;

			/*
			 * Don't check this entry
			 */

			if (tspe == spe)
				continue;

			/*
			 * Don't check invalid entries.
			 */

			if (!ent)
				continue;

			if (strcmp (spw->sp_namp, ent->sp_namp) != 0)
				continue;

			/*
			 * Tell the user this entry is a duplicate of
			 * another and ask them to delete it.
			 */

			puts (_("duplicate shadow password entry\n"));
			printf (_("delete line `%s'? "), spe->line);
			errors++;

			/*
			 * prompt the user to delete the entry or not
			 */

			if (yes_or_no ())
				goto delete_spw;
		}

		/*
		 * Make sure this entry exists in the /etc/passwd
		 * file.
		 */

		if (!pw_locate (spw->sp_namp)) {

			/*
			 * Tell the user this entry has no matching
			 * /etc/passwd entry and ask them to delete it.
			 */

			puts (_("no matching password file entry\n"));
			printf (_("delete line `%s'? "), spe->line);
			errors++;

			/*
			 * prompt the user to delete the entry or not
			 */

			if (yes_or_no ())
				goto delete_spw;
		}

		/*
		 * Warn if last password change in the future.  --marekm
		 */

		if (!quiet && spw->sp_lstchg > time ((time_t *) 0) / SCALE) {
			printf (_
				("user %s: last password change in the future\n"),
				spw->sp_namp);
			errors++;
		}
	}

      shadow_done:

	/*
	 * All done. If there were no deletions we can just abandon any
	 * changes to the files.
	 */

	if (deleted) {
	      write_and_bye:
		if (!pw_close ()) {
			fprintf (stderr, _("%s: cannot update file %s\n"),
				 Prog, pwd_file);
			SYSLOG ((LOG_WARN, "cannot update %s", pwd_file));
			closelog ();
			exit (E_CANTUPDATE);
		}
		if (is_shadow && !spw_close ()) {
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

	if (is_shadow)
		spw_unlock ();
	(void) pw_unlock ();

	/*
	 * Tell the user what we did and exit.
	 */

	if (errors)
		printf (deleted ?
			_("%s: the files have been updated\n") :
			_("%s: no changes\n"), Prog);

	closelog ();
	exit (errors ? E_BADENTRY : E_OKAY);
}
