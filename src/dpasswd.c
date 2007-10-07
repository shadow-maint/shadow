/*
 * Copyright 1990 - 1993, Julianne Frances Haugh
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
 */

#include <config.h>

#include "rcsid.h"
RCSID(PKG_VER "$Id: dpasswd.c,v 1.12 2000/09/02 18:40:43 marekm Exp $")

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include "prototypes.h"
#include "defines.h"
#include "dialup.h"

#define	DTMP	"/etc/d_passwd.tmp"

/*
 * Prompts and messages go here.
 */

#define	DIALCHG		"changed password for %s\n"
#define	DIALADD		"added password for %s\n"
#define	DIALREM		"removed password for %s\n"

static int aflg = 0;
static int dflg = 0;
static char *Prog;

extern int optind;
extern char *optarg;

extern char *getpass();

/* local function prototypes */
static void usage(void);

static void
usage(void)
{
	fprintf(stderr, _("Usage: %s [ -(a|d) ] shell\n"), Prog);
	exit(1);
}

int
main(int argc, char **argv)
{
	struct	dialup	*dial;
	struct	dialup	dent;
	struct	stat	sb;
	FILE	*fp;
	char	*sh = 0;
	char	*cp;
	char	pass[BUFSIZ];
	int	fd;
	int	found = 0;
	int	opt;

	Prog = Basename(argv[0]);

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	OPENLOG(Prog);

	while ((opt = getopt (argc, argv, "a:d:")) != EOF) {
		switch (opt) {
			case 'a':
				aflg++;
				sh = optarg;
				break;
			case 'd':
				dflg++;
				sh = optarg;
				break;
			default:
				usage ();
		}
	}
	if (! aflg && ! dflg)
		aflg++;

	if (! sh) {
		if (optind >= argc)
			usage ();
		else
			sh = argv[optind];
	}
	if (aflg + dflg != 1)
		usage ();

	/*
	 * Add a new shell to the password file, or update an existing
	 * entry.  Begin by getting an encrypted password for this
	 * shell.
	 */

	if (aflg) {
		int	tries = 3;

		dent.du_shell = sh;
		dent.du_passwd = "";  /* XXX warning: const */

again:
		if (! (cp = getpass(_("Shell password: "))))
			exit (1);

		STRFCPY(pass, cp);
		strzero(cp);

		if (! (cp = getpass(_("re-enter Shell password: "))))
			exit (1);

		if (strcmp (pass, cp)) {
			strzero(pass);
			strzero(cp);
			fprintf(stderr,
				_("%s: Passwords do not match, try again.\n"),
				Prog);

			if (--tries)
				goto again;

			exit(1);
		}
		strzero(cp);
		dent.du_passwd = pw_encrypt(pass, crypt_make_salt());
		strzero(pass);
	}

	/*
	 * Create the temporary file for the updated dialup password
	 * information to be placed into.  Turn it into a (FILE *)
	 * for use by putduent().
	 */

	if ((fd = open (DTMP, O_CREAT|O_EXCL|O_RDWR, 0600)) < 0) {
		snprintf(pass, sizeof pass, _("%s: can't create %s"), Prog, DTMP);
		perror (pass);
		exit (1);
	}
	if (! (fp = fdopen (fd, "r+"))) {
		snprintf(pass, sizeof pass, _("%s: can't open %s"), Prog, DTMP);
		perror (pass);
		unlink (DTMP);
		exit (1);
	}

	/*
	 * Scan the dialup password file for the named entry,
	 * copying out other entries along the way.  Copying
	 * stops when a match is found or the file runs out.
	 */

	while ((dial = getduent ())) {
		if (strcmp (dial->du_shell, sh) == 0) {
			found = 1;
			break;
		}
		if (putduent (dial, fp))
			goto failure;
	}

	/*
	 * To delete the entry, just don't copy it.  To update
	 * the entry, output the modified version - works with
	 * new entries as well.
	 */

	if (dflg && ! found) {
		fprintf(stderr, _("%s: Shell %s not found.\n"), Prog, sh);
		goto failure;
	}
	if (aflg)
		if (putduent (&dent, fp))
			goto failure;

	/*
	 * Now copy out the remaining entries.  Flush and close the
	 * new file before doing anything nasty to the existing
	 * file.
	 */


	while ((dial = getduent ()))
		if (putduent (dial, fp))
			goto failure;

	if (fflush (fp))
		goto failure;

	fclose (fp);

	/*
	 * If the original file did not exist, we must create a new
	 * file with owner "root" and mode 400.  Otherwise we copy
	 * the modes from the existing file to the new file.
	 *
	 * After this is done the new file will replace the old file.
	 */

	pwd_init();

	if (! stat (DIALPWD, &sb)) {
		chown (DTMP, sb.st_uid, sb.st_gid);
		chmod (DTMP, sb.st_mode);
		unlink (DIALPWD);
	} else {
		chown (DTMP, 0, 0);
		chmod (DTMP, 0400);
	}
	if (! link (DTMP, DIALPWD))
		unlink (DTMP);

	if (aflg && ! found)
		SYSLOG((LOG_INFO, DIALADD, sh));
	else if (aflg && found)
		SYSLOG((LOG_INFO, DIALCHG, sh));
	else if (dflg)
		SYSLOG((LOG_INFO, DIALREM, sh));

	closelog();
	sync ();
	exit (0);

failure:
	unlink (DTMP);
	closelog();
	exit (1);
}
