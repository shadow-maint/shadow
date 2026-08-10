/*
 * Copyright 1991, 1993, John F. Haugh II and Chip Rosenthal
 * All rights reserved.
 *
 * Permission is granted to copy and create derivative works for any
 * non-commercial purpose, provided this copyright notice is preserved
 * in all copies of source code, or included in human readable form
 * and conspicuously displayed on all copies of object code or
 * distribution media.
 *
 * This software is provided on an AS-IS basis and the author makes
 * no warrantee of any kind.
 */

#ifndef lint
static	char	sccsid[] = "@(#)hushed.c	3.2	22:02:26	02 Jun 1993";
#endif

#include <sys/types.h>
#include <stdio.h>
#ifndef BSD
# include <string.h>
#else
# include <strings.h>
#endif
#include "config.h"
#include "pwd.h"

extern char *getdef_str();

/*
 * hushed - determine if a user receives login messages
 *
 * Look in the hushed-logins file (or user's home directory) to see
 * if the user is to receive the login-time messages.
 */

int
hushed(pw)
struct passwd *pw;
{
	char *hushfile;
	char buf[BUFSIZ];
	int found;
	FILE *fp;

	/*
	 * Get the name of the file to use.  If this option is not
	 * defined, default to a noisy login.
	 */

	if ( (hushfile=getdef_str("HUSHLOGIN_FILE")) == NULL )
		return 0;

	/*
	 * If this is not a fully rooted path then see if the
	 * file exists in the user's home directory.
	 */

	if (hushfile[0] != '/') {
		strcat(strcat(strcpy(buf, pw->pw_dir), "/"), hushfile);
		return (access(buf, 0) == 0);
	}

	/*
	 * If this is a fully rooted path then go through the file
	 * and see if this user is in there.
	 */

	if ((fp = fopen(hushfile, "r")) == NULL)
		return 0;

	for (found = 0;! found && fgets (buf, sizeof buf, fp);) {
		buf[strlen (buf) - 1] = '\0';
		found = ! strcmp (buf,
			buf[0] == '/' ? pw->pw_shell:pw->pw_name);
	}
	(void) fclose(fp);
	return found;
}
