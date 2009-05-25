/*
  vipw, vigr  edit the password or group file
  with -s will edit shadow or gshadow file
 
  Copyright (c) 1997       , Guy Maor <maor@ece.utexas.edu>
  Copyright (c) 1999 - 2000, Marek Michałkiewicz
  Copyright (c) 2002 - 2006, Tomasz Kłoczko
  Copyright (c) 2007 - 2008, Nicolas François
  All rights reserved.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.  */

#include <config.h>

#ident "$Id$"

#include <errno.h>
#include <getopt.h>
#ifdef WITH_SELINUX                                                            
#include <selinux/selinux.h>                                                   
#endif
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utime.h>
#include "defines.h"
#include "groupio.h"
#include "nscd.h"
#include "prototypes.h"
#include "pwio.h"
#include "sgroupio.h"
#include "shadowio.h"
/*@-exitarg@*/
#include "exitcodes.h"

#define MSG_WARN_EDIT_OTHER_FILE _( \
	"You have modified %s.\n"\
	"You may need to modify %s for consistency.\n"\
	"Please use the command '%s' to do so.\n")

/*
 * Global variables
 */
static const char *progname, *filename, *fileeditname;
static bool filelocked = false;
static bool createedit = false;
static int (*unlock) (void);
static bool quiet = false;

/* local function prototypes */
static void usage (void);
static int create_backup_file (FILE *, const char *, struct stat *);
static void vipwexit (const char *msg, int syserr, int ret);
static void vipwedit (const char *, int (*)(void), int (*)(void));

/*
 * usage - display usage message and exit
 */
static void usage (void)
{
	(void) 
	fputs (_("Usage: vipw [options]\n"
	         "\n"
	         "Options:\n"
	         "  -g, --group                   edit group database\n"
	         "  -h, --help                    display this help message and exit\n"
	         "  -p, --passwd                  edit passwd database\n"
	         "  -q, --quiet                   quiet mode\n"
	         "  -s, --shadow                  edit shadow or gshadow database\n"
	         "\n"), stderr);
	exit (E_USAGE);
}

/*
 *
 */
static int create_backup_file (FILE * fp, const char *backup, struct stat *sb)
{
	struct utimbuf ub;
	FILE *bkfp;
	int c;
	mode_t mask;

	mask = umask (077);
	bkfp = fopen (backup, "w");
	(void) umask (mask);
	if (NULL == bkfp) {
		return -1;
	}

	c = 0;
	if (fseeko (fp, 0, SEEK_SET) == 0)
		while ((c = getc (fp)) != EOF) {
			if (putc (c, bkfp) == EOF) {
				break;
			}
		}
	if ((EOF != c) || (ferror (fp) != 0) || (fflush (bkfp) != 0)) {
		fclose (bkfp);
		unlink (backup);
		return -1;
	}
	if (fsync (fileno (bkfp)) != 0) {
		(void) fclose (bkfp);
		unlink (backup);
		return -1;
	}
	if (fclose (bkfp) != 0) {
		unlink (backup);
		return -1;
	}

	ub.actime = sb->st_atime;
	ub.modtime = sb->st_mtime;
	if (   (utime (backup, &ub) != 0)
	    || (chmod (backup, sb->st_mode) != 0)
	    || (chown (backup, sb->st_uid, sb->st_gid) != 0)) {
		unlink (backup);
		return -1;
	}
	return 0;
}

/*
 *
 */
static void vipwexit (const char *msg, int syserr, int ret)
{
	int err = errno;

	if (createedit) {
		if (unlink (fileeditname) != 0) {
			fprintf (stderr, _("%s: failed to remove %s\n"), progname, fileeditname);
			/* continue */
		}
	}
	if (filelocked) {
		if ((*unlock) () == 0) {
			fprintf (stderr, _("%s: failed to unlock %s\n"), progname, fileeditname);
			SYSLOG ((LOG_ERR, "failed to unlock %s", fileeditname));
			/* continue */
		}
	}
	if (NULL != msg) {
		fprintf (stderr, "%s: %s", progname, msg);
	}
	if (0 != syserr) {
		fprintf (stderr, ": %s", strerror (err));
	}
	(void) fputs ("\n", stderr);
	if (!quiet) {
		fprintf (stdout, _("%s: %s is unchanged\n"), progname,
			 filename);
	}
	exit (ret);
}

#ifndef DEFAULT_EDITOR
#define DEFAULT_EDITOR "vi"
#endif

/*
 *
 */
static void
vipwedit (const char *file, int (*file_lock) (void), int (*file_unlock) (void))
{
	const char *editor;
	pid_t pid;
	struct stat st1, st2;
	int status;
	FILE *f;
	char filebackup[1024], fileedit[1024];

	snprintf (filebackup, sizeof filebackup, "%s-", file);
	snprintf (fileedit, sizeof fileedit, "%s.edit", file);
	unlock = file_unlock;
	filename = file;
	fileeditname = fileedit;

	if (access (file, F_OK) != 0) {
		vipwexit (file, 1, 1);
	}
#ifdef WITH_SELINUX
	/* if SE Linux is enabled then set the context of all new files
	   to be the context of the file we are editing */
	if (is_selinux_enabled ()) {
		security_context_t passwd_context=NULL;
		int ret = 0;
		if (getfilecon (file, &passwd_context) < 0) {
			vipwexit (_("Couldn't get file context"), errno, 1);
		}
		ret = setfscreatecon (passwd_context);
		freecon (passwd_context);
		if (0 != ret) {
			vipwexit (_("setfscreatecon () failed"), errno, 1);
		}
	}
#endif
	if (file_lock () == 0) {
		vipwexit (_("Couldn't lock file"), errno, 5);
	}
	filelocked = true;

	/* edited copy has same owners, perm */
	if (stat (file, &st1) != 0) {
		vipwexit (file, 1, 1);
	}
	f = fopen (file, "r");
	if (NULL == f) {
		vipwexit (file, 1, 1);
	}
	if (create_backup_file (f, fileedit, &st1) != 0) {
		vipwexit (_("Couldn't make backup"), errno, 1);
	}
	(void) fclose (f);
	createedit = true;

	editor = getenv ("VISUAL");
	if (NULL == editor) {
		editor = getenv ("EDITOR");
	}
	if (NULL == editor) {
		editor = DEFAULT_EDITOR;
	}

	pid = fork ();
	if (-1 == pid) {
		vipwexit ("fork", 1, 1);
	} else if (0 == pid) {
		/* use the system() call to invoke the editor so that it accepts
		   command line args in the EDITOR and VISUAL environment vars */
		char *buf;

		buf = (char *) malloc (strlen (editor) + strlen (fileedit) + 2);
		snprintf (buf, strlen (editor) + strlen (fileedit) + 2,
			  "%s %s", editor, fileedit);
		if (system (buf) != 0) {
			fprintf (stderr, "%s: %s: %s\n", progname, editor,
				 strerror (errno));
			exit (1);
		} else {
			exit (0);
		}
	}

	for (;;) {
		pid = waitpid (pid, &status, WUNTRACED);
		if ((pid != -1) && (WIFSTOPPED (status) != 0)) {
			/* The child (editor) was suspended.
			 * Suspend vipw. */
			kill (getpid (), WSTOPSIG(status));
			/* wake child when resumed */
			kill (pid, SIGCONT);
		} else {
			break;
		}
	}

	if (   (-1 == pid)
	    || (WIFEXITED (status) == 0)
	    || (WEXITSTATUS (status) != 0)) {
		vipwexit (editor, 1, 1);
	}

	if (stat (fileedit, &st2) != 0) {
		vipwexit (fileedit, 1, 1);
	}
	if (st1.st_mtime == st2.st_mtime) {
		vipwexit (0, 0, 0);
	}
#ifdef WITH_SELINUX                                                            
	/* unset the fscreatecon */                                             
	if (is_selinux_enabled ()) {
		if (setfscreatecon (NULL)) {
			vipwexit (_("setfscreatecon () failed"), errno, 1);
		}
	}
#endif

	/*
	 * XXX - here we should check fileedit for errors; if there are any,
	 * ask the user what to do (edit again, save changes anyway, or quit
	 * without saving). Use pwck or grpck to do the check.  --marekm
	 */
	createedit = false;
	unlink (filebackup);
	link (file, filebackup);
	if (rename (fileedit, file) == -1) {
		fprintf (stderr,
		         _("%s: can't restore %s: %s (your changes are in %s)\n"),
		         progname, file, strerror (errno), fileedit);
		vipwexit (0, 0, 1);
	}

	if ((*file_unlock) () == 0) {
		fprintf (stderr, _("%s: failed to unlock %s\n"), progname, fileeditname);
		SYSLOG ((LOG_ERR, "failed to unlock %s", fileeditname));
		/* continue */
	}
	SYSLOG ((LOG_INFO, "file %s edited", fileeditname));
}

int main (int argc, char **argv)
{
	bool editshadow = false;
	char *a;
	bool do_vipw;

	(void) setlocale (LC_ALL, "");
	(void) bindtextdomain (PACKAGE, LOCALEDIR);
	(void) textdomain (PACKAGE);

	progname = ((a = strrchr (*argv, '/')) ? a + 1 : *argv);
	do_vipw = (strcmp (progname, "vigr") != 0);

	OPENLOG (do_vipw ? "vipw" : "vigr");

	{
		/*
		 * Parse the command line options.
		 */
		int c;
		static struct option long_options[] = {
			{"group", no_argument, NULL, 'g'},
			{"help", no_argument, NULL, 'h'},
			{"passwd", no_argument, NULL, 'p'},
			{"quiet", no_argument, NULL, 'q'},
			{"shadow", no_argument, NULL, 's'},
			{NULL, 0, NULL, '\0'}
		};
		while ((c =
			getopt_long (argc, argv, "ghpqs",
				     long_options, NULL)) != -1) {
			switch (c) {
			case 'g':
				do_vipw = false;
				break;
			case 'h':
				usage ();
				break;
			case 'p':
				do_vipw = true;
				break;
			case 'q':
				quiet = true;
				break;
			case 's':
				editshadow = true;
				break;
			default:
				usage ();
			}
		}
	}

	if (do_vipw) {
		if (editshadow) {
			vipwedit (SHADOW_FILE, spw_lock, spw_unlock);
			printf (MSG_WARN_EDIT_OTHER_FILE,
			        SHADOW_FILE,
			        PASSWD_FILE,
			        "vipw");
		} else {
			vipwedit (PASSWD_FILE, pw_lock, pw_unlock);
			if (spw_file_present ()) {
				printf (MSG_WARN_EDIT_OTHER_FILE,
				        PASSWD_FILE,
				        SHADOW_FILE,
				        "vipw -s");
			}
		}
	} else {
#ifdef SHADOWGRP
		if (editshadow) {
			vipwedit (SGROUP_FILE, sgr_lock, sgr_unlock);
			printf (MSG_WARN_EDIT_OTHER_FILE,
			        SGROUP_FILE,
			        GROUP_FILE,
			        "vigr");
		} else {
#endif
			vipwedit (GROUP_FILE, gr_lock, gr_unlock);
#ifdef SHADOWGRP
			if (sgr_file_present ()) {
				printf (MSG_WARN_EDIT_OTHER_FILE,
				        GROUP_FILE,
				        SGROUP_FILE,
				        "vigr -s");
			}
		}
#endif
	}

	nscd_flush_cache ("passwd");
	nscd_flush_cache ("group");

	return E_SUCCESS;
}

