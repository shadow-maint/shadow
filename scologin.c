/*
 * Copyright 1991, John F. Haugh II and Chip Rosenthal
 * All rights reserved.
 *
 * Permission is granted to copy and create derivative works for any
 * non-commercial purpose, provided this copyright notice is preserved
 * in all copies of source code, or included in human readable form
 * and conspicuously displayed on all copies of object code or
 * distribution media.
 */

#ifndef lint
static	char	sccsid[] = "@(#)scologin.c	3.2	14:38:24	27 Oct 1991";
#endif

#include <stdio.h>
#include "pwd.h"

#define USAGE	"usage: %s [ -r remote_host remote_user local_user [ term_type ] ]\n"
#define LOGIN	"/etc/login"

extern int errno;
extern char *sys_errlist[];
extern char **environ;

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
