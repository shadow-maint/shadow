/*
 * Copyright (c) 1996       , Michael Meskes
 * Copyright (c) 1996 - 2000, Marek Michałkiewicz
 * Copyright (c) 2002 - 2006, Tomasz Kłoczko
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

/*
 * grpunconv - update /etc/group with information from /etc/gshadow.
 *
 */

#include <config.h>

#ident "$Id$"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <grp.h>
#include "nscd.h"
#include "prototypes.h"
#ifdef SHADOWGRP
#include "groupio.h"
#include "sgroupio.h"
/*
 * Global variables
 */
static bool group_locked   = false;
static bool gshadow_locked = false;

/* local function prototypes */
static void fail_exit (int);

static void fail_exit (int status)
{
	if (group_locked) {
		gr_unlock ();
	}
	if (gshadow_locked) {
		sgr_unlock ();
	}
	exit (status);
}

int main (int argc, char **argv)
{
	const struct group *gr;
	struct group grent;
	const struct sgrp *sg;
	char *Prog = argv[0];

	(void) setlocale (LC_ALL, "");
	(void) bindtextdomain (PACKAGE, LOCALEDIR);
	(void) textdomain (PACKAGE);

	if (sgr_file_present () == 0) {
		exit (0);	/* no /etc/gshadow, nothing to do */
	}

	if (gr_lock () == 0) {
		fprintf (stderr, _("%s: can't lock group file\n"), Prog);
		fail_exit (5);
	}
	group_locked = true;
	if (gr_open (O_RDWR) == 0) {
		fprintf (stderr, _("%s: can't open group file\n"), Prog);
		fail_exit (1);
	}

	if (sgr_lock () == 0) {
		fprintf (stderr, _("%s: can't lock shadow group file\n"), Prog);
		fail_exit (5);
	}
	gshadow_locked = true;
	if (sgr_open (O_RDWR) == 0) {
		fprintf (stderr, _("%s: can't open shadow group file\n"), Prog);
		fail_exit (1);
	}

	/*
	 * Update group passwords if non-shadow password is "x".
	 */
	gr_rewind ();
	while ((gr = gr_next ()) != NULL) {
		sg = sgr_locate (gr->gr_name);
		if (   (NULL != sg)
		    && (strcmp (gr->gr_passwd, SHADOW_PASSWD_STRING) == 0)) {
			/* add password to /etc/group */
			grent = *gr;
			grent.gr_passwd = sg->sg_passwd;
			if (gr_update (&grent) == 0) {
				fprintf (stderr,
					 _
					 ("%s: can't update entry for group %s\n"),
					 Prog, grent.gr_name);
				fail_exit (3);
			}
		}
	}

	if (sgr_close () == 0) {
		fprintf (stderr, _("%s: can't update shadow group file\n"),
			 Prog);
		fail_exit (3);
	}

	if (gr_close () == 0) {
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
