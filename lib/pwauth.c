/*
 * Copyright 1992 - 1994, Julianne Frances Haugh
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

#ifndef USE_PAM
#ident "$Id$"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include "prototypes.h"
#include "defines.h"
#include "pwauth.h"
#include "getdef.h"
#ifdef SKEY
#include <skey.h>
#endif
#ifdef __linux__		/* standard password prompt by default */
static const char *PROMPT = gettext_noop ("Password: ");
#else
static const char *PROMPT = gettext_noop ("%s's Password: ");
#endif

int wipe_clear_pass = 1;
char *clear_pass = NULL;

/*
 * pw_auth - perform getpass/crypt authentication
 *
 *	pw_auth gets the user's cleartext password and encrypts it
 *	using the salt in the encrypted password. The results are
 *	compared.
 */

int
pw_auth (const char *cipher, const char *user, int reason, const char *input)
{
	char prompt[1024];
	char *clear = NULL;
	const char *cp;
	int retval;

#ifdef	SKEY
	int use_skey = 0;
	char challenge_info[40];
	struct skey skey;
#endif

	/*
	 * There are programs for adding and deleting authentication data.
	 */

	if (reason == PW_ADD || reason == PW_DELETE)
		return 0;

	/*
	 * There are even programs for changing the user name ...
	 */

	if (reason == PW_CHANGE && input != (char *) 0)
		return 0;

	/*
	 * WARNING:
	 *
	 * When we change a password and we are root, we don't prompt.
	 * This is so root can change any password without having to
	 * know it.  This is a policy decision that might have to be
	 * revisited.
	 */

	if (reason == PW_CHANGE && getuid () == 0)
		return 0;

	/*
	 * WARNING:
	 *
	 * When we are logging in a user with no ciphertext password,
	 * we don't prompt for the password or anything.  In reality
	 * the user could just hit <ENTER>, so it doesn't really
	 * matter.
	 */

	if (cipher == (char *) 0 || *cipher == '\0')
		return 0;

#ifdef	SKEY
	/*
	 * If the user has an S/KEY entry show them the pertinent info
	 * and then we can try validating the created cyphertext and the SKEY.
	 * If there is no SKEY information we default to not using SKEY.
	 */

# ifdef SKEY_BSD_STYLE
	/*
	 * Some BSD updates to the S/KEY API adds a fourth parameter; the
	 * sizeof of the challenge info buffer.
	 */
#  define skeychallenge(s,u,c) skeychallenge(s,u,c,sizeof(c))
# endif

	if (skeychallenge (&skey, user, challenge_info) == 0)
		use_skey = 1;
#endif

	/*
	 * Prompt for the password as required.  FTPD and REXECD both
	 * get the cleartext password for us.
	 */

	if (reason != PW_FTP && reason != PW_REXEC && !input) {
		if (!(cp = getdef_str ("LOGIN_STRING")))
			cp = _(PROMPT);
#ifdef	SKEY
		if (use_skey)
			printf ("[%s]\n", challenge_info);
#endif

		snprintf (prompt, sizeof prompt, cp, user);
		clear = getpass (prompt);
		if (!clear) {
			static char c[1];

			c[0] = '\0';
			clear = c;
		}
		input = clear;
	}

	/*
	 * Convert the cleartext password into a ciphertext string.
	 * If the two match, the return value will be zero, which is
	 * SUCCESS. Otherwise we see if SKEY is being used and check
	 * the results there as well.
	 */

	retval = strcmp (pw_encrypt (input, cipher), cipher);

#ifdef  SKEY
	/*
	 * If (1) The password fails to match, and
	 * (2) The password is empty and
	 * (3) We are using OPIE or S/Key, then
	 * ...Re-prompt, with echo on.
	 * -- AR 8/22/1999
	 */
	if (retval && !input[0] && (use_skey)) {
		clear = getpass (prompt);
		if (!clear) {
			static char c[1];

			c[0] = '\0';
			clear = c;
		}
		input = clear;
	}

	if (retval && use_skey) {
		int passcheck = -1;

		if (skeyverify (&skey, input) == 0)
			passcheck = skey.n;
		if (passcheck > 0)
			retval = 0;
	}
#endif

	/*
	 * Things like RADIUS authentication may need the password -
	 * if the external variable wipe_clear_pass is zero, we will
	 * not wipe it (the caller should wipe clear_pass when it is
	 * no longer needed).  --marekm
	 */

	clear_pass = clear;
	if (wipe_clear_pass && clear && *clear)
		strzero (clear);
	return retval;
}
#endif				/* !USE_PAM */
