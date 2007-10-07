/*
 * grpunconv - update /etc/group with information from /etc/gshadow.
 *
 * Copyright (C) 1996, Michael Meskes <meskes@debian.org>
 * using sources from Marek Michalkiewicz
 * <marekm@i17linuxb.ists.pwr.wroc.pl>
 * This program may be freely used and distributed. If you improve
 * it, please send me your changes. Thanks!
 */

#include <config.h>

#include "rcsid.h"
RCSID (PKG_VER "$Id: grpunconv.c,v 1.11 2002/01/05 15:41:43 kloczek Exp $")
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <grp.h>
#include "prototypes.h"
#ifdef SHADOWGRP
#include "groupio.h"
#include "sgroupio.h"
static int group_locked = 0;
static int gshadow_locked = 0;

/* local function prototypes */
static void fail_exit (int);

static void fail_exit (int status)
{
	if (group_locked)
		gr_unlock ();
	if (gshadow_locked)
		sgr_unlock ();
	exit (status);
}

int main (int argc, char **argv)
{
	const struct group *gr;
	struct group grent;
	const struct sgrp *sg;
	char *Prog = argv[0];

	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);

	if (!sgr_file_present ())
		exit (0);	/* no /etc/gshadow, nothing to do */

	if (!gr_lock ()) {
		fprintf (stderr, _("%s: can't lock group file\n"), Prog);
		fail_exit (5);
	}
	group_locked++;
	if (!gr_open (O_RDWR)) {
		fprintf (stderr, _("%s: can't open group file\n"), Prog);
		fail_exit (1);
	}

	if (!sgr_lock ()) {
		fprintf (stderr, _("%s: can't lock shadow group file\n"),
			 Prog);
		fail_exit (5);
	}
	gshadow_locked++;
	if (!sgr_open (O_RDWR)) {
		fprintf (stderr, _("%s: can't open shadow group file\n"),
			 Prog);
		fail_exit (1);
	}

	/*
	 * Update group passwords if non-shadow password is "x".
	 */
	gr_rewind ();
	while ((gr = gr_next ())) {
		sg = sgr_locate (gr->gr_name);
		if (sg
		    && strcmp (gr->gr_passwd, SHADOW_PASSWD_STRING) == 0) {
			/* add password to /etc/group */
			grent = *gr;
			grent.gr_passwd = sg->sg_passwd;
			if (!gr_update (&grent)) {
				fprintf (stderr,
					 _
					 ("%s: can't update entry for group %s\n"),
					 Prog, grent.gr_name);
				fail_exit (3);
			}
		}
	}

	if (!sgr_close ()) {
		fprintf (stderr, _("%s: can't update shadow group file\n"),
			 Prog);
		fail_exit (3);
	}

	if (!gr_close ()) {
		fprintf (stderr, _("%s: can't update group file\n"), Prog);
		fail_exit (3);
	}

	if (unlink (SGROUP_FILE) != 0) {
		fprintf (stderr, _("%s: can't delete shadow group file\n"),
			 Prog);
		fail_exit (3);
	}

	sgr_unlock ();
	gr_unlock ();
	return 0;
}
#else				/* !SHADOWGRP */
int main (int argc, char **argv)
{
	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);

	fprintf (stderr,
		 _("%s: not configured for shadow group support.\n"),
		 argv[0]);
	exit (1);
}
#endif				/* !SHADOWGRP */
