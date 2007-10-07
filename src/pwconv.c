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
 * Copyright (C) 1996-1997, Marek Michalkiewicz
 * <marekm@i17linuxb.ists.pwr.wroc.pl>
 * This program may be freely used and distributed for any purposes.  If you
 * improve it, please send me your changes. Thanks!
 */

#include <config.h>

#include "rcsid.h"
RCSID (PKG_VER "$Id: pwconv.c,v 1.17 2005/05/25 18:20:25 kloczek Exp $")
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <pwd.h>
#include "prototypes.h"
#include "defines.h"
#include "pwio.h"
#include "shadowio.h"
#include "getdef.h"
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
static int
 shadow_locked = 0, passwd_locked = 0;

/* local function prototypes */
static void fail_exit (int);

static void fail_exit (int status)
{
	if (shadow_locked)
		spw_unlock ();
	if (passwd_locked)
		pw_unlock ();
	exit (status);
}

int main (int argc, char **argv)
{
	const struct passwd *pw;
	struct passwd pwent;
	const struct spwd *sp;
	struct spwd spent;
	char *Prog = argv[0];

	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);

	if (!pw_lock ()) {
		fprintf (stderr, _("%s: can't lock passwd file\n"), Prog);
		fail_exit (E_PWDBUSY);
	}
	passwd_locked++;
	if (!pw_open (O_RDWR)) {
		fprintf (stderr, _("%s: can't open passwd file\n"), Prog);
		fail_exit (E_MISSING);
	}

	if (!spw_lock ()) {
		fprintf (stderr, _("%s: can't lock shadow file\n"), Prog);
		fail_exit (E_PWDBUSY);
	}
	shadow_locked++;
	if (!spw_open (O_CREAT | O_RDWR)) {
		fprintf (stderr, _("%s: can't open shadow file\n"), Prog);
		fail_exit (E_FAILURE);
	}

	/*
	 * Remove /etc/shadow entries for users not in /etc/passwd.
	 */
	spw_rewind ();
	while ((sp = spw_next ())) {
		if (pw_locate (sp->sp_namp))
			continue;

		if (!spw_remove (sp->sp_namp)) {
			/*
			 * This shouldn't happen (the entry exists) but...
			 */
			fprintf (stderr,
				 _
				 ("%s: can't remove shadow entry for %s\n"),
				 Prog, sp->sp_namp);
			fail_exit (E_FAILURE);
		}
	}

	/*
	 * Update shadow entries which don't have "x" as pw_passwd. Add any
	 * missing shadow entries.
	 */
	pw_rewind ();
	while ((pw = pw_next ())) {
		sp = spw_locate (pw->pw_name);
		if (sp) {
			/* do we need to update this entry? */
			if (strcmp (pw->pw_passwd, SHADOW_PASSWD_STRING) == 0)
				continue;
			/* update existing shadow entry */
			spent = *sp;
		} else {
			/* add new shadow entry */
			memset (&spent, 0, sizeof spent);
			spent.sp_namp = pw->pw_name;
			spent.sp_min = getdef_num ("PASS_MIN_DAYS", -1);
			spent.sp_max = getdef_num ("PASS_MAX_DAYS", -1);
			spent.sp_warn = getdef_num ("PASS_WARN_AGE", -1);
			spent.sp_inact = -1;
			spent.sp_expire = -1;
			spent.sp_flag = -1;
		}
		spent.sp_pwdp = pw->pw_passwd;
		spent.sp_lstchg = time ((time_t *) 0) / (24L * 3600L);
		if (!spw_update (&spent)) {
			fprintf (stderr,
				 _
				 ("%s: can't update shadow entry for %s\n"),
				 Prog, spent.sp_namp);
			fail_exit (E_FAILURE);
		}
		/* remove password from /etc/passwd */
		pwent = *pw;
		pwent.pw_passwd = SHADOW_PASSWD_STRING;	/* XXX warning: const */
		if (!pw_update (&pwent)) {
			fprintf (stderr,
				 _
				 ("%s: can't update passwd entry for %s\n"),
				 Prog, pwent.pw_name);
			fail_exit (E_FAILURE);
		}
	}

	if (!spw_close ()) {
		fprintf (stderr, _("%s: can't update shadow file\n"), Prog);
		fail_exit (E_FAILURE);
	}
	if (!pw_close ()) {
		fprintf (stderr, _("%s: can't update passwd file\n"), Prog);
		fail_exit (E_FAILURE);
	}
	chmod (PASSWD_FILE "-", 0600);	/* /etc/passwd- (backup file) */
	spw_unlock ();
	pw_unlock ();
	exit (E_SUCCESS);
}
