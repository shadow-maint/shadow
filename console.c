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
static	char	sccsid[] = "@(#)console.c	3.1	07:47:49	17 Sep 1991";
#endif

#include <stdio.h>
#ifndef BSD
# include <string.h>
#else
# include <strings.h>
#endif

extern	char	*getdef_str();

/*
 * tty - return 1 if the "tty" is a console device, else 0.
 *
 * Note - we need to take extreme care here to avoid locking out root logins
 * if something goes awry.  That's why we do things like call everything a
 * console if the consoles file can't be opened.  Because of this, we must
 * warn the user to protect against the remove of the consoles file since
 * that would allow an unauthorized root login.
 */

int
console (tty)
char	*tty;
{
	FILE	*fp;
	char	buf[BUFSIZ], *console, *s;

	/*
	 * If the CONSOLE configuration definition isn't given, call
	 * everything a valid console.
	 */

	if ((console = getdef_str("CONSOLE")) == NULL)
		return 1;

	/*
	 * If this isn't a filename, then it is a ":" delimited list of
	 * console devices upon which root logins are allowed.
	 */

	if (*console != '/') {
		console = strcpy(buf,console);
		while ((s = strtok(console,":")) != NULL) {
			if (strcmp(s,tty) == 0)
				return 1;

			console = NULL;
		}
		return 0;
	}

	/*
	 * If we can't open the console list, then call everything a
	 * console - otherwise root will never be allowed to login.
	 */

	if ((fp = fopen(console,"r")) == NULL)
		return 1;

	/*
	 * See if this tty is listed in the console file.
	 */

	while (fgets(buf,sizeof(buf),fp) != NULL) {
		buf[strlen(buf)-1] = '\0';
		if (strcmp(buf,tty) == 0) {
			(void) fclose(fp);
			return 1;
		}
	}

	/*
	 * This tty isn't a console.
	 */

	(void) fclose(fp);
	return 0;
}
