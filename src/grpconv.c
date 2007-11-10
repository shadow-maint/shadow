/*
 * grpconv - create or update /etc/gshadow with information from
 * /etc/group.
 *
 * Copyright (C) 1996, Marek Michalkiewicz
 * <marekm@i17linuxb.ists.pwr.wroc.pl>
 * This program may be freely used and distributed. If you improve
 * it, please send me your changes. Thanks!
 */

#include <config.h>
#ident "$Id$"

#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "nscd.h"
#include "prototypes.h"
#ifdef SHADOWGRP
#include "groupio.h"
#include "sgroupio.h"
/*
 * Global variables
 */
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
	struct sgrp sgent;
	char *Prog = argv[0];

	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);

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
		fprintf (stderr, _("%s: can't lock shadow group file\n"), Prog);
		fail_exit (5);
	}
	gshadow_locked++;
	if (!sgr_open (O_CREAT | O_RDWR)) {
		fprintf (stderr, _("%s: can't open shadow group file\n"), Prog);
		fail_exit (1);
	}

	/*
	 * Remove /etc/gshadow entries for groups not in /etc/group.
	 */
	sgr_rewind ();
	while ((sg = sgr_next ())) {
		if (gr_locate (sg->sg_name))
			continue;

		if (!sgr_remove (sg->sg_name)) {
			/*
			 * This shouldn't happen (the entry exists) but...
			 */
			fprintf (stderr,
				 _("%s: can't remove shadow group %s\n"),
				 Prog, sg->sg_name);
			fail_exit (3);
		}
	}

	/*
	 * Update shadow group passwords if non-shadow password is not "x".
	 * Add any missing shadow group entries.
	 */
	gr_rewind ();
	while ((gr = gr_next ())) {
		sg = sgr_locate (gr->gr_name);
		if (sg) {
			/* update existing shadow group entry */
			sgent = *sg;
			if (strcmp (gr->gr_passwd, SHADOW_PASSWD_STRING) != 0)
				sgent.sg_passwd = gr->gr_passwd;
		} else {
			static char *empty = 0;

			/* add new shadow group entry */
			memset (&sgent, 0, sizeof sgent);
			sgent.sg_name = gr->gr_name;
			sgent.sg_passwd = gr->gr_passwd;
			sgent.sg_adm = &empty;
		}
		/*
		 * XXX - sg_mem is redundant, it is currently always a copy
		 * of gr_mem. Very few programs actually use sg_mem, and all
		 * of them are in the shadow suite. Maybe this field could
		 * be used for something else? Any suggestions?
		 */
		sgent.sg_mem = gr->gr_mem;

		if (!sgr_update (&sgent)) {
			fprintf (stderr,
				 _
				 ("%s: can't update shadow entry for %s\n"),
				 Prog, sgent.sg_name);
			fail_exit (3);
		}
		/* remove password from /etc/group */
		grent = *gr;
		grent.gr_passwd = SHADOW_PASSWD_STRING;	/* XXX warning: const */
		if (!gr_update (&grent)) {
			fprintf (stderr,
				 _
				 ("%s: can't update entry for group %s\n"),
				 Prog, grent.gr_name);
			fail_exit (3);
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
	sgr_unlock ();
	gr_unlock ();

	nscd_flush_cache ("group");

	return 0;
}
#else				/* !SHADOWGRP */
int main (int argc, char **argv)
{
	fprintf (stderr,
		 "%s: not configured for shadow group support.\n", argv[0]);
	exit (1);
}
#endif				/* !SHADOWGRP */
