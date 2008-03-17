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

#ident "$Id$"

#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include "chkname.h"
#include "commonio.h"
#include "defines.h"
#include "groupio.h"
#include "nscd.h"
#include "prototypes.h"

#ifdef SHADOWGRP
#include "sgroupio.h"
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
 * Global variables
 */
static char *Prog;
static const char *grp_file = GROUP_FILE;
static int use_system_grp_file = 1;

#ifdef	SHADOWGRP
static const char *sgr_file = SGROUP_FILE;
static int use_system_sgr_file = 1;
static int is_shadow = 0;
#endif
/* Options */
static int read_only = 0;
static int sort_mode = 0;

/* local function prototypes */
static void usage (void);
static void delete_member (char **, const char *);
static void process_flags (int argc, char **argv);
static void open_files (void);
static void close_files (int changed);
static int check_members (const char *groupname,
                          char **members,
                          const char *fmt_info,
                          const char *fmt_prompt,
                          const char *fmt_syslog,
                          int *errors);
static void check_grp_file (int *errors, int *changed);
#ifdef SHADOWGRP
static void compare_members_lists (const char *groupname,
                                   char **members,
                                   char **other_members,
                                   const char *file,
                                   const char *other_file);
static void check_sgr_file (int *errors, int *changed);
#endif

/*
 * usage - print syntax message and exit
 */
static void usage (void)
{
#ifdef	SHADOWGRP
	fprintf (stderr, _("Usage: %s [-r] [-s] [group [gshadow]]\n"), Prog);
#else
	fprintf (stderr, _("Usage: %s [-r] [-s] [group]\n"), Prog);
#endif
	exit (E_USAGE);
}

/*
 * delete_member - delete an entry in a list of members
 */
static void delete_member (char **list, const char *member)
{
	int i;

	for (i = 0; list[i]; i++) {
		if (list[i] == member) {
			break;
		}
	}

	if (list[i]) {
		for (; list[i]; i++) {
			list[i] = list[i + 1];
		}
	}
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
		fprintf (stderr, _("%s: -s and -r are incompatibile\n"), Prog);
		exit (E_USAGE);
	}

	/*
	 * Make certain we have the right number of arguments
	 */
#ifdef	SHADOWGRP
	if ((argc < optind) || (argc > (optind + 2)))
#else
	if ((argc < optind) || (argc > (optind + 1)))
#endif
	{
		usage ();
	}

	/*
	 * If there are two left over filenames, use those as the group and
	 * group password filenames.
	 */
	if (optind != argc) {
		grp_file = argv[optind];
		gr_name (grp_file);
		use_system_grp_file = 0;
	}
#ifdef	SHADOWGRP
	if ((optind + 2) == argc) {
		sgr_file = argv[optind + 1];
		sgr_name (sgr_file);
		is_shadow = 1;
		use_system_sgr_file = 0;
	} else if (optind == argc) {
		is_shadow = sgr_file_present ();
	}
#endif
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
		if (gr_lock () == 0) {
			fprintf (stderr, _("%s: cannot lock file %s\n"),
			         Prog, grp_file);
			if (use_system_grp_file) {
				SYSLOG ((LOG_WARN, "cannot lock %s", grp_file));
			}
			closelog ();
			exit (E_CANT_LOCK);
		}
#ifdef	SHADOWGRP
		if (is_shadow && (sgr_lock () == 0)) {
			fprintf (stderr, _("%s: cannot lock file %s\n"),
			         Prog, sgr_file);
			if (use_system_sgr_file) {
				SYSLOG ((LOG_WARN, "cannot lock %s", sgr_file));
			}
			closelog ();
			exit (E_CANT_LOCK);
		}
#endif
	}

	/*
	 * Open the files. Use O_RDONLY if we are in read_only mode,
	 * O_RDWR otherwise.
	 */
	if (gr_open (read_only ? O_RDONLY : O_RDWR) == 0) {
		fprintf (stderr, _("%s: cannot open file %s\n"), Prog,
		         grp_file);
		if (use_system_grp_file) {
			SYSLOG ((LOG_WARN, "cannot open %s", grp_file));
		}
		closelog ();
		exit (E_CANT_OPEN);
	}
#ifdef	SHADOWGRP
	if (is_shadow && (sgr_open (read_only ? O_RDONLY : O_RDWR) == 0)) {
		fprintf (stderr, _("%s: cannot open file %s\n"), Prog,
		         sgr_file);
		if (use_system_sgr_file) {
			SYSLOG ((LOG_WARN, "cannot open %s", sgr_file));
		}
		closelog ();
		exit (E_CANT_OPEN);
	}
#endif
}

/*
 * close_files - close and unlock the group/gshadow databases
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
		if (gr_close () == 0) {
			fprintf (stderr, _("%s: cannot update file %s\n"),
			         Prog, grp_file);
			exit (E_CANT_UPDATE);
		}
#ifdef	SHADOWGRP
		if (is_shadow && (sgr_close () == 0)) {
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
	if (is_shadow) {
		sgr_unlock ();
	}
#endif
	(void) gr_unlock ();
}

/*
 * check_members - check that every members of a group exist
 *
 *	If an error is detected, *errors is incremented.
 *
 *	The user will be prompted for the removal of the non-existent
 *	user.
 *
 *	If any changes are performed, the return value will be 1,
 *	otherwise check_members() returns 0.
 *
 *	fmt_info, fmt_prompt, and fmt_syslog are used for logging.
 *	  * fmt_info must contain two string flags (%s): for the group's
 *	    name and the missing member.
 *	  * fmt_prompt must contain one string flags (%s): the missing
 *	    member.
 *	  * fmt_syslog must contain two string flags (%s): for the
 *	    group's name and the missing member.
 */
static int check_members (const char *groupname,
                          char **members,
                          const char *fmt_info,
                          const char *fmt_prompt,
                          const char *fmt_syslog,
                          int *errors)
{
	int i;
	int members_changed = 0;

	/*
	 * Make sure each member exists
	 */
	for (i = 0; NULL != members[i]; i++) {
		/* local, no need for xgetpwnam */
		if (getpwnam (members[i]) != NULL) {
			continue;
		}
		/*
		 * Can't find this user. Remove them
		 * from the list.
		 */
		*errors += 1;
		printf (fmt_info, groupname, members[i]);
		printf (fmt_prompt, members[i]);

		if (!yes_or_no (read_only)) {
			continue;
		}

		SYSLOG ((LOG_INFO, fmt_syslog, members[i], groupname));
		members_changed = 1;
		delete_member (members, members[i]);
	}

	return members_changed;
}

#ifdef SHADOWGRP
/*
 * compare_members_lists - make sure the list of members is contained in
 *                         another list.
 *
 *	compare_members_lists() checks that all the members of members are
 *	also in other_members.
 *	file and other_file are used for logging.
 *
 *	TODO: No changes are performed on the lists.
 */
static void compare_members_lists (const char *groupname,
                                   char **members,
                                   char **other_members,
                                   const char *file,
                                   const char *other_file)
{
	char **pmem, **other_pmem;

	for (pmem = members; NULL != *pmem; pmem++) {
		for (other_pmem = other_members; NULL != *other_pmem; other_pmem++) {
			if (strcmp (*pmem, *other_pmem) == 0) {
				break;
			}
		}
		if (*other_pmem == NULL) {
			printf
			    ("'%s' is a member of the '%s' group in %s but not in %s\n",
			     *pmem, groupname, file, other_file);
		}
	}
}
#endif				/* SHADOWGRP */

/*
 * check_grp_file - check the content of the group file
 */
static void check_grp_file (int *errors, int *changed)
{
	struct commonio_entry *gre, *tgre;
	struct group *grp;
#ifdef SHADOWGRP
	struct sgrp *sgr;
#endif

	/*
	 * Loop through the entire group file.
	 */
	for (gre = __gr_get_head (); NULL != gre; gre = gre->next) {
		/*
		 * Skip all NIS entries.
		 */

		if ((gre->line[0] == '+') || (gre->line[0] == '-')) {
			continue;
		}

		/*
		 * Start with the entries that are completely corrupt. They
		 * have no (struct group) entry because they couldn't be
		 * parsed properly.
		 */
		if (NULL == gre->eptr) {

			/*
			 * Tell the user this entire line is bogus and ask
			 * them to delete it.
			 */
			puts (_("invalid group file entry"));
			printf (_("delete line '%s'? "), gre->line);
			*errors += 1;

			/*
			 * prompt the user to delete the entry or not
			 */
			if (!yes_or_no (read_only)) {
				continue;
			}

			/*
			 * All group file deletions wind up here. This code
			 * removes the current entry from the linked list.
			 * When done, it skips back to the top of the loop
			 * to try out the next list element.
			 */
		      delete_gr:
			SYSLOG ((LOG_INFO, "delete group line `%s'",
			         gre->line));
			*changed = 1;

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
		for (tgre = __gr_get_head (); NULL != tgre; tgre = tgre->next) {

			const struct group *ent = tgre->eptr;

			/*
			 * Don't check this entry
			 */
			if (tgre == gre) {
				continue;
			}

			/*
			 * Don't check invalid entries.
			 */
			if (NULL == ent) {
				continue;
			}

			if (strcmp (grp->gr_name, ent->gr_name) != 0) {
				continue;
			}

			/*
			 * Tell the user this entry is a duplicate of
			 * another and ask them to delete it.
			 */
			puts (_("duplicate group entry"));
			printf (_("delete line '%s'? "), gre->line);
			*errors += 1;

			/*
			 * prompt the user to delete the entry or not
			 */
			if (yes_or_no (read_only)) {
				goto delete_gr;
			}
		}

		/*
		 * Check for invalid group names.  --marekm
		 */
		if (!check_group_name (grp->gr_name)) {
			*errors += 1;
			printf (_("invalid group name '%s'\n"), grp->gr_name);
		}

		/*
		 * Workaround for a NYS libc 5.3.12 bug on RedHat 4.2 -
		 * groups with no members are returned as groups with one
		 * member "", causing grpck to fail.  --marekm
		 */
		if (   (NULL != grp->gr_mem[0])
		    && (NULL == grp->gr_mem[1])
		    && ('\0' == grp->gr_mem[0][0])) {
			grp->gr_mem[0] = NULL;
		}

		if (check_members (grp->gr_name, grp->gr_mem,
		                   _("group %s: no user %s\n"),
		                   _("delete member '%s'? "),
		                   "delete member `%s' from group `%s'",
		                   errors) == 1) {
			*changed = 1;
			gre->changed = 1;
			__gr_set_changed ();
		}

#ifdef	SHADOWGRP
		/*
		 * Make sure this entry exists in the /etc/gshadow file.
		 */

		if (is_shadow) {
			sgr = (struct sgrp *) sgr_locate (grp->gr_name);
			if (sgr == NULL) {
				printf (_
				        ("no matching group file entry in %s\n"),
				        sgr_file);
				printf (_("add group '%s' in %s ?"),
				        grp->gr_name, sgr_file);
				*errors += 1;
				if (yes_or_no (read_only)) {
					struct sgrp sg;
					struct group gr;
					static char *empty = NULL;

					sg.sg_name = grp->gr_name;
					sg.sg_passwd = grp->gr_passwd;
					sg.sg_adm = &empty;
					sg.sg_mem = grp->gr_mem;
					SYSLOG ((LOG_INFO,
					         "add group `%s' to `%s'",
					         grp->gr_name, sgr_file));
					*changed = 1;

					if (sgr_update (&sg) == 0) {
						fprintf (stderr,
						         _
						         ("%s: can't update shadow entry for %s\n"),
						         Prog, sg.sg_name);
						exit (E_CANT_UPDATE);
					}
					/* remove password from /etc/group */
					gr = *grp;
					gr.gr_passwd = SHADOW_PASSWD_STRING;	/* XXX warning: const */
					if (gr_update (&gr) == 0) {
						fprintf (stderr,
						         _
						         ("%s: can't update entry for group %s\n"),
						         Prog, gr.gr_name);
						exit (E_CANT_UPDATE);
					}
				}
			} else {
				/**
				 * Verify that all the members defined in /etc/group are also
				 * present in /etc/gshadow.
				 */
				compare_members_lists (grp->gr_name,
				                       grp->gr_mem, sgr->sg_mem,
				                       grp_file, sgr_file);
			}
		}
#endif

	}
}

#ifdef SHADOWGRP
/*
 * check_sgr_file - check the content of the shadowed group file (gshadow)
 */
static void check_sgr_file (int *errors, int *changed)
{
	struct group *grp;
	struct commonio_entry *sge, *tsge;
	struct sgrp *sgr;

	/*
	 * Loop through the entire shadow group file.
	 */
	for (sge = __sgr_get_head (); NULL != sge; sge = sge->next) {

		/*
		 * Start with the entries that are completely corrupt. They
		 * have no (struct sgrp) entry because they couldn't be
		 * parsed properly.
		 */
		if (NULL == sge->eptr) {

			/*
			 * Tell the user this entire line is bogus and ask
			 * them to delete it.
			 */
			puts (_("invalid shadow group file entry"));
			printf (_("delete line '%s'? "), sge->line);
			*errors += 1;

			/*
			 * prompt the user to delete the entry or not
			 */
			if (!yes_or_no (read_only)) {
				continue;
			}

			/*
			 * All shadow group file deletions wind up here. 
			 * This code removes the current entry from the
			 * linked list. When done, it skips back to the top
			 * of the loop to try out the next list element.
			 */
		      delete_sg:
			SYSLOG ((LOG_INFO, "delete shadow line `%s'",
			         sge->line));
			*changed = 1;

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
		for (tsge = __sgr_get_head (); NULL != tsge; tsge = tsge->next) {

			const struct sgrp *ent = tsge->eptr;

			/*
			 * Don't check this entry
			 */
			if (tsge == sge) {
				continue;
			}

			/*
			 * Don't check invalid entries.
			 */
			if (NULL == ent) {
				continue;
			}

			if (strcmp (sgr->sg_name, ent->sg_name) != 0) {
				continue;
			}

			/*
			 * Tell the user this entry is a duplicate of
			 * another and ask them to delete it.
			 */
			puts (_("duplicate shadow group entry"));
			printf (_("delete line '%s'? "), sge->line);
			*errors += 1;

			/*
			 * prompt the user to delete the entry or not
			 */
			if (yes_or_no (read_only)) {
				goto delete_sg;
			}
		}

		/*
		 * Make sure this entry exists in the /etc/group file.
		 */
		grp = (struct group *) gr_locate (sgr->sg_name);
		if (grp == NULL) {
			printf (_("no matching group file entry in %s\n"),
			        grp_file);
			printf (_("delete line '%s'? "), sge->line);
			*errors += 1;
			if (yes_or_no (read_only)) {
				goto delete_sg;
			}
		} else {
			/**
			 * Verify that the all members defined in /etc/gshadow are also
			 * present in /etc/group.
			 */
			compare_members_lists (sgr->sg_name,
			                       sgr->sg_mem, grp->gr_mem,
			                       sgr_file, grp_file);
		}

		/*
		 * Make sure each administrator exists
		 */
		if (check_members (sgr->sg_name, sgr->sg_adm,
		                   _("shadow group %s: no administrative user %s\n"),
		                   _("delete administrative member '%s'? "),
		                   "delete admin `%s' from shadow group `%s'",
		                   errors) == 1) {
			*changed = 1;
			sge->changed = 1;
			__sgr_set_changed ();
		}

		/*
		 * Make sure each member exists
		 */
		if (check_members (sgr->sg_name, sgr->sg_mem,
		                   _("shadow group %s: no user %s\n"),
		                   _("delete member '%s'? "),
		                   "delete member `%s' from shadow group `%s'",
		                   errors) == 1) {
			*changed = 1;
			sge->changed = 1;
			__sgr_set_changed ();
		}
	}
}
#endif				/* SHADOWGRP */

/*
 * grpck - verify group file integrity
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

	OPENLOG ("grpck");

	/* Parse the command line arguments */
	process_flags (argc, argv);

	open_files ();

	if (sort_mode) {
		gr_sort ();
#ifdef	SHADOWGRP
		if (is_shadow) {
			sgr_sort ();
		}
		changed = 1;
#endif
	} else {
		check_grp_file (&errors, &changed);
#ifdef	SHADOWGRP
		if (is_shadow) {
			check_sgr_file (&errors, &changed);
		}
#endif
	}

	/* Commit the change in the database if needed */
	close_files (changed);

	nscd_flush_cache ("group");

	/*
	 * Tell the user what we did and exit.
	 */
	if (errors != 0) {
		printf (changed ?
		        _("%s: the files have been updated\n") :
		        _("%s: no changes\n"), Prog);
	}

	exit (errors ? E_BAD_ENTRY : E_OKAY);
}

