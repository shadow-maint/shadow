/*
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
 * pwconv - create or update /etc/shadow with information from
 * /etc/passwd.
 *
 * It is more like SysV pwconv, slightly different from the original Shadow
 * pwconv. Depends on "x" as password in /etc/passwd which means that the
 * password has already been moved to /etc/shadow. There is no need to move
 * /etc/npasswd to /etc/passwd, password files are updated using library
 * routines with proper locking.
 *
 * Can be used to update /etc/shadow after adding/deleting users by editing
 * /etc/passwd. There is no man page yet, but this program should be close
 * to pwconv(1M) on Solaris 2.x.
 *
 * Warning: make sure that all users have "x" as the password in /etc/passwd
 * before running this program for the first time on a system which already
 * has shadow passwords. Anything else (like "*" from old versions of the
 * shadow suite) will replace the user's encrypted password in /etc/shadow.
 *
 * Doesn't currently support pw_age information in /etc/passwd, and doesn't
 * support DBM files. Add it if you need it...
 *
 */

#include <config.h>

#ident "$Id$"

#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "defines.h"
#include "getdef.h"
#include "prototypes.h"
#include "pwio.h"
#include "shadowio.h"
#include "nscd.h"
/*
 * exit status values
 */
#define E_SUCCESS	0	/* success */
#define E_NOPERM	1	/* permission denied */
#define E_USAGE		2	/* invalid command syntax */
#define E_FAILURE	3	/* unexpected failure, nothing done */
#define E_MISSING	4	/* unexpected failure, passwd file missing */
#define E_PWDBUSY	5	/* passwd file(s) busy */
#define E_BADENTRY	6	/* bad shadow entry */
/*
 * Global variables
 */
static bool shadow_locked = false;
static bool passwd_locked = false;

/* local function prototypes */
static void fail_exit (int);

static void fail_exit (int status)
{
	if (shadow_locked) {
		spw_unlock ();
	}
	if (passwd_locked) {
		pw_unlock ();
	}
	exit (status);
}

int main (int argc, char **argv)
{
	const struct passwd *pw;
	struct passwd pwent;
	const struct spwd *sp;
	struct spwd spent;
	char *Prog = argv[0];

	(void) setlocale (LC_ALL, "");
	(void) bindtextdomain (PACKAGE, LOCALEDIR);
	(void) textdomain (PACKAGE);

	if (pw_lock () == 0) {
		fprintf (stderr,
		         _("%s: cannot lock %s\n"), Prog, pw_dbname ());
		fail_exit (E_PWDBUSY);
	}
	passwd_locked = true;
	if (pw_open (O_RDWR) == 0) {
		fprintf (stderr,
		         _("%s: cannot open %s\n"), Prog, pw_dbname ());
		fail_exit (E_MISSING);
	}

	if (spw_lock () == 0) {
		fprintf (stderr,
		         _("%s: cannot lock %s\n"), Prog, spw_dbname ());
		fail_exit (E_PWDBUSY);
	}
	shadow_locked = true;
	if (spw_open (O_CREAT | O_RDWR) == 0) {
		fprintf (stderr,
		         _("%s: cannot open %s\n"), Prog, spw_dbname ());
		fail_exit (E_FAILURE);
	}

	/*
	 * Remove /etc/shadow entries for users not in /etc/passwd.
	 */
	spw_rewind ();
	while ((sp = spw_next ()) != NULL) {
		if (pw_locate (sp->sp_namp) != NULL) {
			continue;
		}

		if (spw_remove (sp->sp_namp) == 0) {
			/*
			 * This shouldn't happen (the entry exists) but...
			 */
			fprintf (stderr,
			         _("%s: cannot remove entry '%s' from %s\n"),
			         Prog, sp->sp_namp, spw_dbname ());
			fail_exit (E_FAILURE);
		}
	}

	/*
	 * Update shadow entries which don't have "x" as pw_passwd. Add any
	 * missing shadow entries.
	 */
	pw_rewind ();
	while ((pw = pw_next ()) != NULL) {
		sp = spw_locate (pw->pw_name);
		if (NULL != sp) {
			/* do we need to update this entry? */
			if (strcmp (pw->pw_passwd, SHADOW_PASSWD_STRING) == 0) {
				continue;
			}
			/* update existing shadow entry */
			spent = *sp;
		} else {
			/* add new shadow entry */
			memset (&spent, 0, sizeof spent);
			spent.sp_namp   = pw->pw_name;
			spent.sp_min    = getdef_num ("PASS_MIN_DAYS", -1);
			spent.sp_max    = getdef_num ("PASS_MAX_DAYS", -1);
			spent.sp_warn   = getdef_num ("PASS_WARN_AGE", -1);
			spent.sp_inact  = -1;
			spent.sp_expire = -1;
			spent.sp_flag   = SHADOW_SP_FLAG_UNSET;
		}
		spent.sp_pwdp = pw->pw_passwd;
		spent.sp_lstchg = (long) time ((time_t *) 0) / SCALE;
		if (spw_update (&spent) == 0) {
			fprintf (stderr,
				 _
				 ("%s: can't update shadow entry for %s\n"),
				 Prog, spent.sp_namp);
			fail_exit (E_FAILURE);
		}

		/* remove password from /etc/passwd */
		pwent = *pw;
		pwent.pw_passwd = SHADOW_PASSWD_STRING;	/* XXX warning: const */
		if (pw_update (&pwent) == 0) {
			fprintf (stderr,
				 _
				 ("%s: can't update passwd entry for %s\n"),
				 Prog, pwent.pw_name);
			fail_exit (E_FAILURE);
		}
	}

	if (spw_close () == 0) {
		fprintf (stderr,
		         _("%s: failure while writing changes to %s\n"),
		         Prog, spw_dbname ());
		fail_exit (E_FAILURE);
	}
	if (pw_close () == 0) {
		fprintf (stderr,
		         _("%s: failure while writing changes to %s\n"),
		         Prog, pw_dbname ());
		fail_exit (E_FAILURE);
	}
	chmod (PASSWD_FILE "-", 0600);	/* /etc/passwd- (backup file) */
	spw_unlock ();
	pw_unlock ();

	nscd_flush_cache ("passwd");

	exit (E_SUCCESS);
}

