/*
 * Copyright (c) 1989 - 1994, Julianne Frances Haugh
 * Copyright (c) 1996 - 2000, Marek Michałkiewicz
 * Copyright (c) 2001 - 2006, Tomasz Kłoczko
 * Copyright (c) 2007 - 2010, Nicolas François
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
 * Separated from setup.c.  --marekm
 */

#include <config.h>

#ident "$Id$"

#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <ctype.h>
#include "prototypes.h"
#include "defines.h"
#include <pwd.h>
#include "getdef.h"

#ifndef USE_PAM
static void
addenv_path (const char *varname, const char *dirname, const char *filename)
{
	char *buf;
	size_t len = strlen (dirname) + strlen (filename) + 2;
	int wlen;

	buf = xmalloc (len);
	wlen = snprintf (buf, len, "%s/%s", dirname, filename);
	assert (wlen == (int) len - 1);

	addenv (varname, buf);
	free (buf);
}

static void read_env_file (const char *filename)
{
	FILE *fp;
	char buf[1024];
	char *cp, *name, *val;

	fp = fopen (filename, "r");
	if (NULL == fp) {
		return;
	}
	while (fgets (buf, (int)(sizeof buf), fp) == buf) {
		cp = strrchr (buf, '\n');
		if (NULL == cp) {
			break;
		}
		*cp = '\0';

		cp = buf;
		/* ignore whitespace and comments */
		while (('\0' != *cp) && isspace (*cp)) {
			cp++;
		}
		if (('\0' == *cp) || ('#' == *cp)) {
			continue;
		}
		/*
		 * ignore lines which don't follow the name=value format
		 * (for example, the "export NAME" shell commands)
		 */
		name = cp;
		while (('\0' != *cp) && !isspace (*cp) && ('=' != *cp)) {
			cp++;
		}
		if ('=' != *cp) {
			continue;
		}
		/* NUL-terminate the name */
		*cp = '\0';
		cp++;
		val = cp;
#if 0				/* XXX untested, and needs rewrite with fewer goto's :-) */
/*
 (state, char_type) -> (state, action)

 state: unquoted, single_quoted, double_quoted, escaped, double_quoted_escaped
 char_type: normal, white, backslash, single, double
 action: remove_curr, remove_curr_skip_next, remove_prev, finish XXX
*/
	      no_quote:
		if (*cp == '\\') {
			/* remove the backslash */
			remove_char (cp);
			/* skip over the next character */
			if (*cp)
				cp++;
			goto no_quote;
		} else if (*cp == '\'') {
			/* remove the quote */
			remove_char (cp);
			/* now within single quotes */
			goto s_quote;
		} else if (*cp == '"') {
			/* remove the quote */
			remove_char (cp);
			/* now within double quotes */
			goto d_quote;
		} else if (*cp == '\0') {
			/* end of string */
			goto finished;
		} else if (isspace (*cp)) {
			/* unescaped whitespace - end of string */
			*cp = '\0';
			goto finished;
		} else {
			cp++;
			goto no_quote;
		}
	      s_quote:
		if (*cp == '\'') {
			/* remove the quote */
			remove_char (cp);
			/* unquoted again */
			goto no_quote;
		} else if (*cp == '\0') {
			/* end of string */
			goto finished;
		} else {
			/* preserve everything within single quotes */
			cp++;
			goto s_quote;
		}
	      d_quote:
		if (*cp == '\"') {
			/* remove the quote */
			remove_char (cp);
			/* unquoted again */
			goto no_quote;
		} else if (*cp == '\\') {
			cp++;
			/* if backslash followed by double quote, remove backslash
			   else skip over the backslash and following char */
			if (*cp == '"')
				remove_char (cp - 1);
			else if (*cp)
				cp++;
			goto d_quote;
		}
		else if (*cp == '\0') {
			/* end of string */
			goto finished;
		} else {
			/* preserve everything within double quotes */
			goto d_quote;
		}
	      finished:
#endif				/* 0 */
		/*
		 * XXX - should handle quotes, backslash escapes, etc.
		 * like the shell does.
		 */
		addenv (name, val);
	}
	(void) fclose (fp);
}
#endif				/* USE_PAM */


/*
 *	change to the user's home directory
 *	set the HOME, SHELL, MAIL, PATH, and LOGNAME or USER environmental
 *	variables.
 */

void setup_env (struct passwd *info)
{
#ifndef USE_PAM
	const char *envf;
#endif
	const char *cp;

	/*
	 * Change the current working directory to be the home directory
	 * of the user.  It is a fatal error for this process to be unable
	 * to change to that directory.  There is no "default" home
	 * directory.
	 *
	 * We no longer do it as root - should work better on NFS-mounted
	 * home directories.  Some systems default to HOME=/, so we make
	 * this a configurable option.  --marekm
	 */

	if (chdir (info->pw_dir) == -1) {
		static char temp_pw_dir[] = "/";

		if (!getdef_bool ("DEFAULT_HOME") || chdir ("/") == -1) {
			fprintf (shadow_logfd, _("Unable to cd to '%s'\n"),
				 info->pw_dir);
			SYSLOG ((LOG_WARN,
				 "unable to cd to `%s' for user `%s'\n",
				 info->pw_dir, info->pw_name));
			closelog ();
			exit (EXIT_FAILURE);
		}
		(void) puts (_("No directory, logging in with HOME=/"));
		free (info->pw_dir);
		info->pw_dir = xstrdup (temp_pw_dir);
	}

	/*
	 * Create the HOME environmental variable and export it.
	 */

	addenv ("HOME", info->pw_dir);

	/*
	 * Create the SHELL environmental variable and export it.
	 */

	if ((NULL == info->pw_shell) || ('\0' == *info->pw_shell)) {
		static char temp_pw_shell[] = SHELL;

		free (info->pw_shell);
		info->pw_shell = xstrdup (temp_pw_shell);
	}

	addenv ("SHELL", info->pw_shell);

	/*
	 * Export the user name.  For BSD derived systems, it's "USER", for
	 * all others it's "LOGNAME".  We set both of them.
	 */

	addenv ("USER", info->pw_name);
	addenv ("LOGNAME", info->pw_name);

	/*
	 * Create the PATH environmental variable and export it.
	 */

	cp = getdef_str ((info->pw_uid == 0) ? "ENV_SUPATH" : "ENV_PATH");

	if (NULL == cp) {
		/* not specified, use a minimal default */
		addenv ((info->pw_uid == 0) ? "PATH=/sbin:/bin:/usr/sbin:/usr/bin" : "PATH=/bin:/usr/bin", NULL);
	} else if (strchr (cp, '=')) {
		/* specified as name=value (PATH=...) */
		addenv (cp, NULL);
	} else {
		/* only value specified without "PATH=" */
		addenv ("PATH", cp);
	}

#ifndef USE_PAM
	/*
	 * Create the MAIL environmental variable and export it.  login.defs
	 * knows the prefix.
	 */

	if (getdef_bool ("MAIL_CHECK_ENAB")) {
		cp = getdef_str ("MAIL_DIR");
		if (NULL != cp) {
			addenv_path ("MAIL", cp, info->pw_name);
		} else {
			cp = getdef_str ("MAIL_FILE");
			if (NULL != cp) {
				addenv_path ("MAIL", info->pw_dir, cp);
			} else {
#if defined(MAIL_SPOOL_FILE)
				addenv_path ("MAIL", info->pw_dir, MAIL_SPOOL_FILE);
#elif defined(MAIL_SPOOL_DIR)
				addenv_path ("MAIL", MAIL_SPOOL_DIR, info->pw_name);
#endif
			}
		}
	}

	/*
	 * Read environment from optional config file.  --marekm
	 */
	envf = getdef_str ("ENVIRON_FILE");
	if (NULL != envf) {
		read_env_file (envf);
	}
#endif				/* !USE_PAM */
}

