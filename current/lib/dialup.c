/*
 * Copyright 1989 - 1991, Julianne Frances Haugh
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
RCSID("$Id: dialup.c,v 1.3 1997/12/07 23:26:50 marekm Exp $")

#include <stdio.h>
#include "prototypes.h"
#include "defines.h"
#include "dialup.h"

static	FILE	*dialpwd;

void
setduent(void)
{
	if (dialpwd)
		rewind (dialpwd);
	else
		dialpwd = fopen (DIALPWD, "r");
}

void
endduent(void)
{
	if (dialpwd)
		fclose (dialpwd);

	dialpwd = (FILE *) 0;
}

struct dialup *
fgetduent(FILE *fp)
{
	static	struct	dialup	dialup;	/* static structure to point to */
	static	char	sh[128];	/* some space for a login shell */
	static	char	passwd[128];	/* some space for dialup password */
	char	buf[BUFSIZ];
	char	*cp;
	char	*cp2;

	if (! fp)
		return 0;

	if (! fp || feof (fp))
		return ((struct dialup *) 0);

	while (fgets (buf, sizeof buf, fp) == buf && buf[0] == '#')
		;

	if (feof (fp))
		return ((struct dialup *) 0);

	if ((cp = strchr (buf, '\n')))
		*cp = '\0';

	if (! (cp = strchr (buf, ':')))
		return ((struct dialup *) 0);

	if (cp - buf > sizeof sh)	/* something is fishy ... */
		return ((struct dialup *) 0);

	*cp++ = '\0';
	(void) strcpy (sh, buf);
	sh[cp - buf] = '\0';

	if ((cp2 = strchr (cp, ':')))
		*cp2 = '\0';

	if (strlen (cp) + 1 > sizeof passwd) /* something is REALLY fishy */
		return ((struct dialup *) 0);

	(void) strcpy (passwd, cp);

	dialup.du_shell = sh;
	dialup.du_passwd = passwd;

	return (&dialup);
}

struct dialup *
getduent(void)
{
	if (! dialpwd)
		setduent ();

	return fgetduent (dialpwd);
}

struct dialup *
getdushell(const char *sh)
{
	struct	dialup	*dialup;

	while ((dialup = getduent ())) {
		if (strcmp (sh, dialup->du_shell) == 0)
			return (dialup);

		if (strcmp (dialup->du_shell, "*") == 0)
			return (dialup);
	}
	return ((struct dialup *) 0);
}

int
isadialup(const char *tty)
{
	FILE	*fp;
	char	buf[BUFSIZ];
	int	dialup = 0;

	if (! (fp = fopen (DIALUPS, "r")))
		return (0);

	while (fgets (buf, sizeof buf, fp) == buf) {
		if (buf[0] == '#')
			continue;

		buf[strlen (buf) - 1] = '\0';

		if (strcmp (buf, tty) == 0) {
			dialup = 1;
			break;
		}
	}
	fclose (fp);

	return (dialup);
}

int
putduent(const struct dialup *dial, FILE *fp)
{
	if (! fp || ! dial)
		return -1;

	if (fprintf (fp, "%s:%s\n", dial->du_shell, dial->du_passwd) == EOF)
		return -1;

	return 0;
}
