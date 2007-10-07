/*
 * Copyright 1989 - 1994, Julianne Frances Haugh
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
 * ARE DISCLAIMED.  IN NO EVENT SHALL JULIE HAUGH OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * pwunconv - restore old password file from shadow password file.
 *
 *	Pwunconv copies the password file information from the shadow
 *	password file, merging entries from an optional existing shadow
 *	file.
 *
 *      Modifed by Guy Maor <maor@debian.org> to acquire necessary locks
 *      and modify the files in place.
 */

#include <config.h>

#include "rcsid.h"
RCSID(PKG_VER "$Id: pwunconv.c,v 1.9 2000/08/26 18:27:18 marekm Exp $")

#include "defines.h"
#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <pwd.h>
#include <unistd.h>
#include "prototypes.h"
#include "pwio.h"
#include "shadowio.h"

#ifndef	SHADOWPWD
int
main(int argc, char **argv)
{
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	fprintf(stderr, _("%s: Shadow passwords are not configured.\n"),
		argv[0]);
	exit(1);
}

#else	/*{*/

char	*l64a ();

static int shadow_locked = 0,
	passwd_locked = 0;

/* local function prototypes */
static void fail_exit(int);

static void
fail_exit(int status)
{
	if (shadow_locked)
		spw_unlock();
	if (passwd_locked)
		pw_unlock();
	exit(status);
}


int
main(int argc, char **argv)
{
	const struct passwd *pw;
	struct passwd pwent;
	const struct spwd *spwd;
#ifdef	ATT_AGE
	char	newage[5];
#endif
	char *Prog = argv[0];

	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);

	if (!spw_file_present())
		/* shadow not installed, do nothing */
		exit(0);

	if (!pw_lock()) {
		fprintf(stderr, _("%s: can't lock passwd file\n"), Prog);
		fail_exit(5);
	}
	passwd_locked++;
	if (!pw_open(O_RDWR)) {
		fprintf(stderr, _("%s: can't open passwd file\n"), Prog);
		fail_exit(1);
	}

	if (!spw_lock()) {
		fprintf(stderr, _("%s: can't open shadow file\n"), Prog);
		fail_exit(5);
	}
	shadow_locked++;
	if (!spw_open(O_RDWR)) {
		fprintf(stderr, _("%s: can't open shadow file\n"), Prog);
		fail_exit(1);
	}

	pw_rewind();
	while ((pw = pw_next())) {
		if (!(spwd = spw_locate(pw->pw_name)))
			continue;

		pwent = *pw;

		/*
		 * Update password if non-shadow is "x".
		 */
		if (strcmp(pw->pw_passwd, SHADOW_PASSWD_STRING) == 0)
			pwent.pw_passwd = spwd->sp_pwdp;

		/*
		 * Password aging works differently in the two different systems.
		 * With shadow password files you apparently must have some aging
		 * information.  The maxweeks or minweeks may not map exactly.
		 * In pwconv we set max == 10000, which is about 30 years.  Here
		 * we have to undo that kludge.  So, if maxdays == 10000, no aging
		 * information is put into the new file.  Otherwise, the days are
		 * converted to weeks and so on.
		 */

#ifdef	ATT_AGE
		if (spwd->sp_max > (63*WEEK/SCALE) && spwd->sp_max < 10000)
			spwd->sp_max = (63*WEEK/SCALE); /* 10000 is infinity */

		if (spwd->sp_min >= 0 && spwd->sp_min <= 63*7 &&
				spwd->sp_max >= 0 && spwd->sp_max <= 63*7) {
			if (spwd->sp_lstchg == -1)
				spwd->sp_lstchg = 0;

			spwd->sp_max /= WEEK/SCALE;	/* turn it into weeks */
			spwd->sp_min /= WEEK/SCALE;
			spwd->sp_lstchg /= WEEK/SCALE;

			strncpy (newage, l64a (spwd->sp_lstchg * (64L*64L) +
				  spwd->sp_min * (64L) + spwd->sp_max), 5);
			pwent.pw_age = newage;
		} else
			pwent.pw_age = "";
#endif	/* ATT_AGE */
		if (!pw_update(&pwent)) {
			fprintf(stderr,
				_("%s: can't update entry for user %s\n"),
				Prog, pwent.pw_name);
			fail_exit(3);
		}
	}

	if (!spw_close()) {
		fprintf(stderr, _("%s: can't update shadow password file\n"), Prog);
		fail_exit(3);
	}

	if (!pw_close()) {
		fprintf(stderr, _("%s: can't update password file\n"), Prog);
		fail_exit(3);
	}

	if (unlink(SHADOW) != 0) {
		fprintf(stderr, _("%s: can't delete shadow password file\n"), Prog);
		fail_exit(3);
	}

	spw_unlock();
	pw_unlock();
	return 0;
}
#endif
