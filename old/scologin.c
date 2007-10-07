/*
 * Copyright 1991, Julianne Frances Haugh and Chip Rosenthal
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

#ifndef lint
static char rcsid[] = "$Id: scologin.c,v 1.1 1997/05/01 23:12:00 marekm Exp $";
#endif

#include <stdio.h>
#include <pwd.h>

#define USAGE	"usage: %s [ -r remote_host remote_user local_user [ term_type ] ]\n"
#define LOGIN	"/etc/login"

extern int errno;
extern char *sys_errlist[];
extern char **environ;

int
main(argc, argv)
int argc;
char *argv[];
{
	char *rhost, *ruser, *luser;
	char term[1024], *nargv[8], *nenvp[2];
	int root_user, i;
	struct passwd *pw;

	if (argc == 1) {

		/*
		 * Called from telnetd.
		 */
		nargv[0] = "login";
		nargv[1] = "-p";
		nargv[2] = NULL;

	} else if (strcmp(argv[1], "-r") == 0 && argc >= 6) {

		/*
		 * Called from rlogind.
		 */

		rhost = argv[2];
		ruser = argv[3];
		luser = argv[4];
		root_user = ((pw = getpwnam(luser)) != NULL && pw->pw_uid == 0);

		i = 0;
		if ( argc == 6 ) {
			strcpy(term, "TERM=");
			strncat(term+sizeof("TERM=")-1,
				argv[5], sizeof(term)-sizeof("TERM="));
			term[sizeof(term)-1] = '\0';
			nenvp[i++] = term;
		}
		nenvp[i++] = NULL;
		environ = nenvp;

		i = 0;
		nargv[i++] = "login";
		nargv[i++] = "-p";
		nargv[i++] = "-h";
		nargv[i++] = rhost;
		if (ruserok(rhost, root_user, ruser, luser) == 0)
			nargv[i++] = "-f";
		nargv[i++] = luser;
		nargv[i++] = NULL;

	} else {

		fprintf(stderr, USAGE, argv[0]);
		exit(1);

	}

	(void) execv(LOGIN, nargv);
	fprintf(stderr, "%s: could not exec '%s' [%s]\n",
		argv[0], LOGIN, sys_errlist[errno]);
	exit(1);
	/*NOTREACHED*/
}
