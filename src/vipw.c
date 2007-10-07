/*
  vipw, vigr  edit the password or group file
  with -s will edit shadow or gshadow file
 
  Copyright (C) 1997 Guy Maor <maor@ece.utexas.edu>

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
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

  */

#include <config.h>

#ident "$Id: vipw.c,v 1.22 2006/06/20 20:00:04 kloczek Exp $"

#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utime.h>
#include "defines.h"
#include "exitcodes.h"
#include "groupio.h"
#include "nscd.h"
#include "prototypes.h"
#include "pwio.h"
#include "sgroupio.h"
#include "shadowio.h"
/*
 * Global variables
 */
static const char *progname, *filename, *fileeditname;
static int filelocked = 0, createedit = 0;
static int (*unlock) (void);
static int quiet = 0;

/* local function prototypes */
static void usage (void);
static int create_backup_file (FILE *, const char *, struct stat *);
static void vipwexit (const char *, int, int);
static void vipwedit (const char *, int (*)(void), int (*)(void));

/*
 * usage - display usage message and exit
 */
static void usage (void)
{
	fprintf (stderr, _("Usage: vipw [options]\n"
			   "\n"
			   "Options:\n"
			   "  -g, --group			edit group database\n"
			   "  -h, --help			display this help message and exit\n"
			   "  -p, --passwd			edit passwd database\n"
			   "  -q, --quiet			quiet mode\n"
			   "  -s, --shadow			edit shadow or gshadow database\n"
			   "\n"));
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
	umask (mask);
	if (!bkfp)
		return -1;

	c = 0;
	if (fseeko (fp, 0, SEEK_SET) == 0)
		while ((c = getc (fp)) != EOF) {
			if (putc (c, bkfp) == EOF)
				break;
		}
	if (c != EOF || ferror (fp) || fflush (bkfp)) {
		fclose (bkfp);
		unlink (backup);
		return -1;
	}
	if (fclose (bkfp)) {
		unlink (backup);
		return -1;
	}

	ub.actime = sb->st_atime;
	ub.modtime = sb->st_mtime;
	if (utime (backup, &ub) ||
	    chmod (backup, sb->st_mode) ||
	    chown (backup, sb->st_uid, sb->st_gid)) {
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

	if (createedit)
		unlink (fileeditname);
	if (filelocked)
		(*unlock) ();
	if (msg)
		fprintf (stderr, "%s: %s", progname, msg);
	if (syserr)
		fprintf (stderr, ": %s", strerror (err));
	if (!quiet)
		fprintf (stdout, _("\n%s: %s is unchanged\n"), progname,
			 filename);
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

	if (access (file, F_OK))
		vipwexit (file, 1, 1);
	if (!file_lock ())
		vipwexit (_("Couldn't lock file"), errno, 5);
	filelocked = 1;

	/* edited copy has same owners, perm */
	if (stat (file, &st1))
		vipwexit (file, 1, 1);
	if (!(f = fopen (file, "r")))
		vipwexit (file, 1, 1);
	if (create_backup_file (f, fileedit, &st1))
		vipwexit (_("Couldn't make backup"), errno, 1);
	createedit = 1;

	editor = getenv ("VISUAL");
	if (!editor)
		editor = getenv ("EDITOR");
	if (!editor)
		editor = DEFAULT_EDITOR;

	if ((pid = fork ()) == -1)
		vipwexit ("fork", 1, 1);
	else if (!pid) {
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
		} else
			exit (0);
	}

	for (;;) {
		pid = waitpid (pid, &status, WUNTRACED);
		if (WIFSTOPPED (status)) {
			kill (getpid (), SIGSTOP);
			kill (getpid (), SIGCONT);
		} else
			break;
	}

	if (pid == -1 || !WIFEXITED (status) || WEXITSTATUS (status))
		vipwexit (editor, 1, 1);

	if (stat (fileedit, &st2))
		vipwexit (fileedit, 1, 1);
	if (st1.st_mtime == st2.st_mtime)
		vipwexit (0, 0, 0);

	/*
	 * XXX - here we should check fileedit for errors; if there are any,
	 * ask the user what to do (edit again, save changes anyway, or quit
	 * without saving). Use pwck or grpck to do the check.  --marekm
	 */
	createedit = 0;
	unlink (filebackup);
	link (file, filebackup);
	if (rename (fileedit, file) == -1) {
		fprintf (stderr,
			 _
			 ("%s: can't restore %s: %s (your changes are in %s)\n"),
			 progname, file, strerror (errno), fileedit);
		vipwexit (0, 0, 1);
	}

	(*file_unlock) ();
}

int main (int argc, char **argv)
{
	int editshadow = 0;
	char *a;
	int do_vipw;

	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);

	progname = ((a = strrchr (*argv, '/')) ? a + 1 : *argv);
	do_vipw = (strcmp (progname, "vigr") != 0);

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
		};
		while ((c =
			getopt_long (argc, argv, "ghpqs",
				     long_options, NULL)) != -1) {
			switch (c) {
			case 'g':
				do_vipw = 0;
				break;
			case 'h':
				usage ();
				break;
			case 'p':
				do_vipw = 1;
				break;
			case 'q':
				quiet = 1;
				break;
			case 's':
				editshadow = 1;
				break;
			default:
				usage ();
			}
		}
	}

	if (do_vipw) {
		if (editshadow)
			vipwedit (SHADOW_FILE, spw_lock, spw_unlock);
		else
			vipwedit (PASSWD_FILE, pw_lock, pw_unlock);
	} else {
#ifdef SHADOWGRP
		if (editshadow)
			vipwedit (SGROUP_FILE, sgr_lock, sgr_unlock);
		else
#endif
			vipwedit (GROUP_FILE, gr_lock, gr_unlock);
	}

	nscd_flush_cache ("passwd");
	nscd_flush_cache ("group");

	exit (E_SUCCESS);
}
