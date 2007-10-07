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
RCSID (PKG_VER "$Id: grpck.c,v 1.20 2002/01/05 15:41:43 kloczek Exp $")
#include <stdio.h>
#include <fcntl.h>
#include <grp.h>
#include "prototypes.h"
#include "defines.h"
#include "chkname.h"
#include <pwd.h>
#include "commonio.h"
#include "groupio.h"
extern void __gr_del_entry (const struct commonio_entry *);
extern struct commonio_entry *__gr_get_head (void);

#ifdef SHADOWGRP
#include "sgroupio.h"
extern void __sgr_del_entry (const struct commonio_entry *);
extern struct commonio_entry *__sgr_get_head (void);
#endif

/*
 * Exit codes
 */

#define	E_OKAY		0
#define	E_USAGE		1
#define	E_BAD_ENTRY	2
#define	E_CANT_OPEN	3
#define	E_CANT_LOCK	4
#define	E_CANT_UPDATE	5

/*
 * Local variables
 */

static char *Prog;
static const char *grp_file = GROUP_FILE;

#ifdef	SHADOWGRP
static const char *sgr_file = SGROUP_FILE;
#endif
static int read_only = 0;

/* local function prototypes */
static void usage (void);
static int yes_or_no (void);
static void delete_member (char **, const char *);

/*
 * usage - print syntax message and exit
 */

static void usage (void)
{
#ifdef	SHADOWGRP
	fprintf (stderr, _("Usage: %s [-r] [-s] [group [gshadow]]\n"),
		 Prog);
#else
	fprintf (stderr, _("Usage: %s [-r] [-s] [group]\n"), Prog);
#endif
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
 * delete_member - delete an entry in a list of members
 */

static void delete_member (char **list, const char *member)
{
	int i;

	for (i = 0; list[i]; i++)
		if (list[i] == member)
			break;

	if (list[i])
		for (; list[i]; i++)
			list[i] = list[i + 1];
}

/*
 * grpck - verify group file integrity
 */

int main (int argc, char **argv)
{
	int arg;
	int errors = 0;
	int deleted = 0;
	int i;
	struct commonio_entry *gre, *tgre;
	struct group *grp;
	int sort_mode = 0;

#ifdef	SHADOWGRP
	struct commonio_entry *sge, *tsge;
	struct sgrp *sgr;
	int is_shadow = 0;
#endif

	/*
	 * Get my name so that I can use it to report errors.
	 */

	Prog = Basename (argv[0]);

	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);

	OPENLOG (Prog);

	/*
	 * Parse the command line arguments
	 */

	while ((arg = getopt (argc, argv, "qrs")) != EOF) {
		switch (arg) {
		case 'q':
			/* quiet - ignored for now */
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
		fprintf (stderr, _("%s: -s and -r are incompatibile\n"),
			 Prog);
		exit (E_USAGE);
	}

	/*
	 * Make certain we have the right number of arguments
	 */

#ifdef	SHADOWGRP
	if (optind != argc && optind + 1 != argc && optind + 2 != argc)
#else
	if (optind != argc && optind + 1 != argc)
#endif
		usage ();

	/*
	 * If there are two left over filenames, use those as the group and
	 * group password filenames.
	 */

	if (optind != argc) {
		grp_file = argv[optind];
		gr_name (grp_file);
	}
#ifdef	SHADOWGRP
	if (optind + 2 == argc) {
		sgr_file = argv[optind + 1];
		sgr_name (sgr_file);
		is_shadow = 1;
	} else if (optind == argc)
		is_shadow = sgr_file_present ();
#endif

	/*
	 * Lock the files if we aren't in "read-only" mode
	 */

	if (!read_only) {
		if (!gr_lock ()) {
			fprintf (stderr, _("%s: cannot lock file %s\n"),
				 Prog, grp_file);
			if (optind == argc)
				SYSLOG ((LOG_WARN, "cannot lock %s",
					 grp_file));
			closelog ();
			exit (E_CANT_LOCK);
		}
#ifdef	SHADOWGRP
		if (is_shadow && !sgr_lock ()) {
			fprintf (stderr, _("%s: cannot lock file %s\n"),
				 Prog, sgr_file);
			if (optind == argc)
				SYSLOG ((LOG_WARN, "cannot lock %s",
					 sgr_file));
			closelog ();
			exit (E_CANT_LOCK);
		}
#endif
	}

	/*
	 * Open the files. Use O_RDONLY if we are in read_only mode,
	 * O_RDWR otherwise.
	 */

	if (!gr_open (read_only ? O_RDONLY : O_RDWR)) {
		fprintf (stderr, _("%s: cannot open file %s\n"), Prog,
			 grp_file);
		if (optind == argc)
			SYSLOG ((LOG_WARN, "cannot open %s", grp_file));
		closelog ();
		exit (E_CANT_OPEN);
	}
#ifdef	SHADOWGRP
	if (is_shadow && !sgr_open (read_only ? O_RDONLY : O_RDWR)) {
		fprintf (stderr, _("%s: cannot open file %s\n"), Prog,
			 sgr_file);
		if (optind == argc)
			SYSLOG ((LOG_WARN, "cannot open %s", sgr_file));
		closelog ();
		exit (E_CANT_OPEN);
	}
#endif

	if (sort_mode) {
		gr_sort ();
#ifdef	SHADOWGRP
		if (is_shadow)
			sgr_sort ();
#endif
		goto write_and_bye;
	}

	/*
	 * Loop through the entire group file.
	 */

	for (gre = __gr_get_head (); gre; gre = gre->next) {
		/*
		 * Skip all NIS entries.
		 */

		if (gre->line[0] == '+' || gre->line[0] == '-')
			continue;

		/*
		 * Start with the entries that are completely corrupt. They
		 * have no (struct group) entry because they couldn't be
		 * parsed properly.
		 */

		if (!gre->eptr) {

			/*
			 * Tell the user this entire line is bogus and ask
			 * them to delete it.
			 */

			printf (_("invalid group file entry\n"));
			printf (_("delete line `%s'? "), gre->line);
			errors++;

			/*
			 * prompt the user to delete the entry or not
			 */

			if (!yes_or_no ())
				continue;

			/*
			 * All group file deletions wind up here. This code
			 * removes the current entry from the linked list.
			 * When done, it skips back to the top of the loop
			 * to try out the next list element.
			 */

		      delete_gr:
			SYSLOG ((LOG_INFO, "delete group line `%s'",
				 gre->line));
			deleted++;

			__gr_del_entry (gre);
			continue;
		}

		/*
		 * Group structure is good, start using it.
		 */

		grp = gre->eptr;

		/*
		 * Make sure this entry has a unique name.
		 */

		for (tgre = __gr_get_head (); tgre; tgre = tgre->next) {

			const struct group *ent = tgre->eptr;

			/*
			 * Don't check this entry
			 */

			if (tgre == gre)
				continue;

			/*
			 * Don't check invalid entries.
			 */

			if (!ent)
				continue;

			if (strcmp (grp->gr_name, ent->gr_name) != 0)
				continue;

			/*
			 * Tell the user this entry is a duplicate of
			 * another and ask them to delete it.
			 */

			puts (_("duplicate group entry\n"));
			printf (_("delete line `%s'? "), gre->line);
			errors++;

			/*
			 * prompt the user to delete the entry or not
			 */

			if (yes_or_no ())
				goto delete_gr;
		}

		/*
		 * Check for invalid group names.  --marekm
		 */
		if (!check_group_name (grp->gr_name)) {
			errors++;
			printf (_("invalid group name `%s'\n"),
				grp->gr_name);
		}

		/*
		 * Workaround for a NYS libc 5.3.12 bug on RedHat 4.2 -
		 * groups with no members are returned as groups with one
		 * member "", causing grpck to fail.  --marekm
		 */

		if (grp->gr_mem[0] && !grp->gr_mem[1]
		    && *(grp->gr_mem[0]) == '\0')
			grp->gr_mem[0] = (char *) 0;

		/*
		 * Make sure each member exists
		 */

		for (i = 0; grp->gr_mem[i]; i++) {
			if (getpwnam (grp->gr_mem[i]))
				continue;
			/*
			 * Can't find this user. Remove them
			 * from the list.
			 */

			errors++;
			printf (_("group %s: no user %s\n"),
				grp->gr_name, grp->gr_mem[i]);
			printf (_("delete member `%s'? "), grp->gr_mem[i]);

			if (!yes_or_no ())
				continue;

			SYSLOG ((LOG_INFO, "delete member `%s' group `%s'",
				 grp->gr_mem[i], grp->gr_name));
			deleted++;
			delete_member (grp->gr_mem, grp->gr_mem[i]);
			gre->changed = 1;
			__gr_set_changed ();
		}
	}

#ifdef	SHADOWGRP
	if (!is_shadow)
		goto shadow_done;

	/*
	 * Loop through the entire shadow group file.
	 */

	for (sge = __sgr_get_head (); sge; sge = sge->next) {

		/*
		 * Start with the entries that are completely corrupt. They
		 * have no (struct sgrp) entry because they couldn't be
		 * parsed properly.
		 */

		if (!sge->eptr) {

			/*
			 * Tell the user this entire line is bogus and ask
			 * them to delete it.
			 */

			printf (_("invalid shadow group file entry\n"));
			printf (_("delete line `%s'? "), sge->line);
			errors++;

			/*
			 * prompt the user to delete the entry or not
			 */

			if (!yes_or_no ())
				continue;

			/*
			 * All shadow group file deletions wind up here. 
			 * This code removes the current entry from the
			 * linked list. When done, it skips back to the top
			 * of the loop to try out the next list element.
			 */

		      delete_sg:
			SYSLOG ((LOG_INFO, "delete shadow line `%s'",
				 sge->line));
			deleted++;

			__sgr_del_entry (sge);
			continue;
		}

		/*
		 * Shadow group structure is good, start using it.
		 */

		sgr = sge->eptr;

		/*
		 * Make sure this entry has a unique name.
		 */

		for (tsge = __sgr_get_head (); tsge; tsge = tsge->next) {

			const struct sgrp *ent = tsge->eptr;

			/*
			 * Don't check this entry
			 */

			if (tsge == sge)
				continue;

			/*
			 * Don't check invalid entries.
			 */

			if (!ent)
				continue;

			if (strcmp (sgr->sg_name, ent->sg_name) != 0)
				continue;

			/*
			 * Tell the user this entry is a duplicate of
			 * another and ask them to delete it.
			 */

			puts (_("duplicate shadow group entry\n"));
			printf (_("delete line `%s'? "), sge->line);
			errors++;

			/*
			 * prompt the user to delete the entry or not
			 */

			if (yes_or_no ())
				goto delete_sg;
		}

		/*
		 * Make sure this entry exists in the /etc/group file.
		 */

		if (!gr_locate (sgr->sg_name)) {
			puts (_("no matching group file entry\n"));
			printf (_("delete line `%s'? "), sge->line);
			errors++;
			if (yes_or_no ())
				goto delete_sg;
		}

		/*
		 * Make sure each administrator exists
		 */

		for (i = 0; sgr->sg_adm[i]; i++) {
			if (getpwnam (sgr->sg_adm[i]))
				continue;
			/*
			 * Can't find this user. Remove them
			 * from the list.
			 */

			errors++;
			printf (_
				("shadow group %s: no administrative user %s\n"),
				sgr->sg_name, sgr->sg_adm[i]);
			printf (_("delete administrative member `%s'? "),
				sgr->sg_adm[i]);

			if (!yes_or_no ())
				continue;

			SYSLOG ((LOG_INFO,
				 "delete admin `%s' from shadow group `%s'",
				 sgr->sg_adm[i], sgr->sg_name));
			deleted++;
			delete_member (sgr->sg_adm, sgr->sg_adm[i]);
			sge->changed = 1;
			__sgr_set_changed ();
		}

		/*
		 * Make sure each member exists
		 */

		for (i = 0; sgr->sg_mem[i]; i++) {
			if (getpwnam (sgr->sg_mem[i]))
				continue;

			/*
			 * Can't find this user. Remove them from the list.
			 */

			errors++;
			printf (_("shadow group %s: no user %s\n"),
				sgr->sg_name, sgr->sg_mem[i]);
			printf (_("delete member `%s'? "), sgr->sg_mem[i]);

			if (!yes_or_no ())
				continue;

			SYSLOG ((LOG_INFO,
				 "delete member `%s' from shadow group `%s'",
				 sgr->sg_mem[i], sgr->sg_name));
			deleted++;
			delete_member (sgr->sg_mem, sgr->sg_mem[i]);
			sge->changed = 1;
			__sgr_set_changed ();
		}
	}

      shadow_done:
#endif				/* SHADOWGRP */

	/*
	 * All done. If there were no deletions we can just abandon any
	 * changes to the files.
	 */

	if (deleted) {
	      write_and_bye:
		if (!gr_close ()) {
			fprintf (stderr, _("%s: cannot update file %s\n"),
				 Prog, grp_file);
			exit (E_CANT_UPDATE);
		}
#ifdef	SHADOWGRP
		if (is_shadow && !sgr_close ()) {
			fprintf (stderr, _("%s: cannot update file %s\n"),
				 Prog, sgr_file);
			exit (E_CANT_UPDATE);
		}
#endif
	}

	/*
	 * Don't be anti-social - unlock the files when you're done.
	 */

#ifdef	SHADOWGRP
	if (is_shadow)
		sgr_unlock ();
#endif
	(void) gr_unlock ();

	/*
	 * Tell the user what we did and exit.
	 */

	if (errors)
#ifdef NDBM
		printf (deleted ?
			_
			("%s: the files have been updated; run mkpasswd\n")
			: _("%s: no changes\n"), Prog);
#else
		printf (deleted ?
			_("%s: the files have been updated\n") :
			_("%s: no changes\n"), Prog);
#endif

	exit (errors ? E_BAD_ENTRY : E_OKAY);
}
