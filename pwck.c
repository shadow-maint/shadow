/*
 * Copyright 1992, 1993, John F. Haugh II
 * All rights reserved.
 *
 * Permission is granted to copy and create derivative works for any
 * non-commercial purpose, provided this copyright notice is preserved
 * in all copies of source code, or included in human readable form
 * and conspicuously displayed on all copies of object code or
 * distribution media.
 *
 * This software is provided on an AS-IS basis and the author makes
 * no warrantee of any kind.
 */

#ifndef	lint
static	char	sccsid[] = "@(#)pwck.c	3.2	17:42:44	01 May 1993";
#endif

#include <stdio.h>
#include <fcntl.h>
#include <grp.h>
#ifdef	BSD
#include <strings.h>
#else
#include <string.h>
#endif

#include "config.h"
#include "pwd.h"
#ifdef	SHADOWPWD
#include "shadow.h"
#endif

#ifdef	USE_SYSLOG
#include <syslog.h>

#ifndef	LOG_WARN
#define	LOG_WARN LOG_WARNING
#endif
#endif

struct	pw_file_entry {
	char	*pwf_line;
	int	pwf_changed;
	struct	passwd	*pwf_entry;
	struct	pw_file_entry *pwf_next;
};

#ifdef	SHADOWPWD
struct	spw_file_entry {
	char	*spwf_line;
	int	spwf_changed;
	struct	spwd	*spwf_entry;
	struct	spw_file_entry *spwf_next;
};
#endif

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
 * Message strings
 */

char	*CANTOPEN = "%s: cannot open file %s\n";
char	*CANTLOCK = "%s: cannot lock file %s\n";
char	*CANTUPDATE = "%s: cannot update file %s\n";
char	*CHANGES = "%s: the files have been updated; run mkpasswd\n";
char	*NOCHANGES = "%s: no changes\n";
char	*NOGROUP = "user %s: no group %d\n";
char	*NOHOME = "user %s: directory %s does not exist\n";
char	*NOSHELL = "user %s: program %s does not exist\n";
char	*BADENTRY = "invalid password file entry\n";
char	*PWDUP = "duplicate password entry\n";
char	*DELETE = "delete line `%s'? ";
char	*NO = "No";
#ifdef	SHADOWPWD
char	*BADSENTRY = "invalid shadow password file entry\n";
char	*SPWDUP = "duplicate shadow password entry\n";
#endif

/*
 * Global variables
 */

extern	int	optind;
extern	char	*optarg;
extern	struct	pw_file_entry	*__pwf_head;
extern	int	__pw_changed;
#ifdef	SHADOWPWD
extern	struct	spw_file_entry	*__spwf_head;
extern	int	__sp_changed;
#endif

/*
 * Local variables
 */

char	*Prog;
char	*pwd_file = PWDFILE;
#ifdef	SHADOWPWD
char	*spw_file = SHADOW;
#endif
char	read_only;

/*
 * usage - print syntax message and exit
 */

usage ()
{
#ifdef	SHADOWPWD
	fprintf (stderr, "Usage: %s [ -r ] [ passwd shadow ]\n", Prog);
#else
	fprintf (stderr, "Usage: %s [ -r ] [ passwd ]\n", Prog);
#endif
	exit (E_USAGE);
}

/*
 * yes_or_no - get answer to question from the user
 */

int
yes_or_no ()
{
	char	buf[BUFSIZ];

	/*
	 * In read-only mode all questions are answered "no".
	 */

	if (read_only) {
		puts (NO);
		return 0;
	}

	/*
	 * Get a line and see what the first character is.
	 */

	if (fgets (buf, BUFSIZ, stdin))
		return buf[0] == 'y' || buf[0] == 'Y';

	return 0;
}

/*
 * pwck - verify password file integrity
 */

main (argc, argv)
int	argc;
char	**argv;
{
	int	arg;
	int	errors = 0;
	int	deleted = 0;
	struct	pw_file_entry	*pfe, *tpfe;
	struct	passwd	*pwd;
#ifdef	SHADOWPWD
	struct	spw_file_entry	*spe, *tspe;
	struct	spwd	*spw;
#endif

	/*
	 * Get my name so that I can use it to report errors.
	 */

	if (Prog = strrchr (argv[0], '/'))
		Prog++;
	else
		Prog = argv[0];

#ifdef	USE_SYSLOG
	openlog (Prog, LOG_PID|LOG_CONS|LOG_NOWAIT, LOG_AUTH);
#endif

	/*
	 * Parse the command line arguments
	 */

	while ((arg = getopt (argc, argv, "r")) != EOF) {
		if (arg == 'r')
			read_only = 1;
		else if (arg != EOF)
			usage ();
	}

	/*
	 * Make certain we have the right number of arguments
	 */

#ifdef	SHADOWPWD
	if (optind != argc && optind + 2 != argc)
#else
	if (optind != argc && optind + 1 != argc)
#endif
		usage ();

	/*
	 * If there are two left over filenames, use those as the
	 * password and shadow password filenames.
	 */

	if (optind != argc) {
		pwd_file = argv[optind];
		pw_name (pwd_file);
#ifdef	SHADOWPWD
		spw_file = argv[optind + 1];
		spw_name (spw_file);
#endif
	}

	/*
	 * Lock the files if we aren't in "read-only" mode
	 */

	if (! read_only) {
		if (! pw_lock ()) {
			fprintf (stderr, CANTLOCK, Prog, pwd_file);
#ifdef	USE_SYSLOG
			if (optind == argc)
				syslog (LOG_WARN, "cannot lock %s\n", pwd_file);

			closelog ();
#endif
			exit (E_CANTLOCK);
		}
#ifdef	SHADOWPWD
		if (! spw_lock ()) {
			fprintf (stderr, CANTLOCK, Prog, spw_file);
#ifdef	USE_SYSLOG
			if (optind == argc)
				syslog (LOG_WARN, "cannot lock %s\n", spw_file);

			closelog ();
#endif
			exit (E_CANTLOCK);
		}
#endif
	}

	/*
	 * Open the files.  Use O_RDONLY if we are in read_only mode,
	 * O_RDWR otherwise.
	 */

	if (! pw_open (read_only ? O_RDONLY:O_RDWR)) {
		fprintf (stderr, CANTOPEN, Prog, pwd_file);
#ifdef	USE_SYSLOG
		if (optind == argc)
			syslog (LOG_WARN, "cannot open %s\n", pwd_file);

		closelog ();
#endif
		exit (E_CANTOPEN);
	}
#ifdef	SHADOWPWD
	if (! spw_open (read_only ? O_RDONLY:O_RDWR)) {
		fprintf (stderr, CANTOPEN, Prog, spw_file);
#ifdef	USE_SYSLOG
		if (optind == argc)
			syslog (LOG_WARN, "cannot open %s\n", spw_file);

		closelog ();
#endif
		exit (E_CANTOPEN);
	}
#endif

	/*
	 * Loop through the entire password file.
	 */

	for (pfe = __pwf_head;pfe;pfe = pfe->pwf_next) {

		/*
		 * Start with the entries that are completely corrupt.
		 * They have no (struct passwd) entry because they couldn't
		 * be parsed properly.
		 */

		if (pfe->pwf_entry == (struct passwd *) 0) {

			/*
			 * Tell the user this entire line is bogus and
			 * ask them to delete it.
			 */

			printf (BADENTRY);
			printf (DELETE, pfe->pwf_line);
			errors++;

			/*
			 * prompt the user to delete the entry or not
			 */

			if (! yes_or_no ())
				continue;

			/*
			 * All password file deletions wind up here.  This
			 * code removes the current entry from the linked
			 * list.  When done, it skips back to the top of
			 * the loop to try out the next list element.
			 */

delete_pw:
#ifdef	USE_SYSLOG
			syslog (LOG_INFO,
				"delete passwd line `%s'\n", pfe->pwf_line);
#endif
			deleted++;
			__pw_changed = 1;

			/*
			 * Simple case - delete from the head of the
			 * list.
			 */

			if (pfe == __pwf_head) {
				__pwf_head = pfe->pwf_next;
				continue;
			}

			/*
			 * Hard case - find entry where pwf_next is
			 * the current entry.
			 */

			for (tpfe = __pwf_head;tpfe->pwf_next != pfe;
					tpfe = pfe->pwf_next)
				;

			tpfe->pwf_next = pfe->pwf_next;
			continue;
		}

		/*
		 * Password structure is good, start using it.
		 */

		pwd = pfe->pwf_entry;

		/*
		 * Make sure this entry has a unique name.
		 */

		for (tpfe = __pwf_head;tpfe;tpfe = tpfe->pwf_next) {

			/*
			 * Don't check this entry
			 */

			if (tpfe == pfe)
				continue;

			/*
			 * Don't check invalid entries.
			 */

			if (tpfe->pwf_entry == (struct passwd *) 0)
				continue;

			if (strcmp (pwd->pw_name, tpfe->pwf_entry->pw_name))
				continue;

			/*
			 * Tell the user this entry is a duplicate of
			 * another and ask them to delete it.
			 */

			puts (PWDUP);
			printf (DELETE, pfe->pwf_line);
			errors++;

			/*
			 * prompt the user to delete the entry or not
			 */

			if (yes_or_no ())
				goto delete_pw;
		}

		/*
		 * Make sure the primary group exists
		 */

		if (! getgrgid (pwd->pw_gid)) {

			/*
			 * No primary group, just give a warning
			 */

			printf (NOGROUP, pwd->pw_name, pwd->pw_gid);
			errors++;
		}

		/*
		 * Make sure the home directory exists
		 */

		if (access (pwd->pw_dir, 0)) {

			/*
			 * Home directory doesn't exist, give a warning
			 */

			printf (NOHOME, pwd->pw_name, pwd->pw_dir);
			errors++;
		}

		/*
		 * Make sure the login shell is executable
		 */

		if (pwd->pw_shell[0] && access (pwd->pw_shell, 0)) {

			/*
			 * Login shell doesn't exist, give a warning
			 */
			
			printf (NOSHELL, pwd->pw_name, pwd->pw_shell);
			errors++;
		}
	}

#ifdef	SHADOWPWD
	/*
	 * Loop through the entire shadow password file.
	 */

	for (spe = __spwf_head;spe;spe = spe->spwf_next) {

		/*
		 * Start with the entries that are completely corrupt.
		 * They have no (struct spwd) entry because they couldn't
		 * be parsed properly.
		 */

		if (spe->spwf_entry == (struct spwd *) 0) {

			/*
			 * Tell the user this entire line is bogus and
			 * ask them to delete it.
			 */

			printf (BADSENTRY);
			printf (DELETE, spe->spwf_line);
			errors++;

			/*
			 * prompt the user to delete the entry or not
			 */

			if (! yes_or_no ())
				continue;

			/*
			 * All shadow file deletions wind up here.  This
			 * code removes the current entry from the linked
			 * list.  When done, it skips back to the top of
			 * the loop to try out the next list element.
			 */

delete_spw:
#ifdef	USE_SYSLOG
			syslog (LOG_INFO,
				"delete shadow line `%s'\n", spe->spwf_line);
#endif
			deleted++;
			__sp_changed = 1;

			/*
			 * Simple case - delete from the head of the
			 * list.
			 */

			if (spe == __spwf_head) {
				__spwf_head = spe->spwf_next;
				continue;
			}

			/*
			 * Hard case - find entry where spwf_next is
			 * the current entry.
			 */

			for (tspe = __spwf_head;tspe->spwf_next != spe;
					tspe = spe->spwf_next)
				;

			tspe->spwf_next = spe->spwf_next;
			continue;
		}

		/*
		 * Shadow password structure is good, start using it.
		 */

		spw = spe->spwf_entry;

		/*
		 * Make sure this entry has a unique name.
		 */

		for (tspe = __spwf_head;tspe;tspe = tspe->spwf_next) {

			/*
			 * Don't check this entry
			 */

			if (tspe == spe)
				continue;

			/*
			 * Don't check invalid entries.
			 */

			if (tspe->spwf_entry == (struct spwd *) 0)
				continue;

			if (strcmp (spw->sp_namp, tspe->spwf_entry->sp_namp))
				continue;

			/*
			 * Tell the user this entry is a duplicate of
			 * another and ask them to delete it.
			 */

			puts (SPWDUP);
			printf (DELETE, spe->spwf_line);
			errors++;

			/*
			 * prompt the user to delete the entry or not
			 */

			if (yes_or_no ())
				goto delete_spw;
		}
	}
#endif

	/*
	 * All done.  If there were no deletions we can just abandon any
	 * changes to the files.
	 */

	if (deleted) {
		if (! pw_close ()) {
			fprintf (stderr, CANTUPDATE, Prog, pwd_file);
#ifdef	USE_SYLOG
			syslog (LOG_WARN, "cannot update %s\n", pwd_file);
			closelog ();
#endif
			exit (E_CANTUPDATE);
		}
#ifdef	SHADOWPWD
		if (! spw_close ()) {
			fprintf (stderr, CANTUPDATE, Prog, spw_file);
#ifdef	USE_SYLOG
			syslog (LOG_WARN, "cannot update %s\n", spw_file);
			closelog ();
#endif
			exit (E_CANTUPDATE);
		}
#endif
	}

	/*
	 * Don't be anti-social - unlock the files when you're done.
	 */

#ifdef	SHADOWPWD
	(void) spw_unlock ();
#endif
	(void) pw_unlock ();

	/*
	 * Tell the user what we did and exit.
	 */

	if (errors)
		printf (deleted ? CHANGES:NOCHANGES, Prog);

#ifdef	USE_SYSLOG
	closelog ();
#endif
	exit (errors ? E_BADENTRY:E_OKAY);
}
