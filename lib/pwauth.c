/*
 * SPDX-FileCopyrightText: 1992 - 1994, Julianne Frances Haugh
 * SPDX-FileCopyrightText: 1996 - 2000, Marek Michałkiewicz
 * SPDX-FileCopyrightText: 2003 - 2006, Tomasz Kłoczko
 * SPDX-FileCopyrightText: 2008 - 2009, Nicolas François
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
#include "memzero.h"
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

bool wipe_clear_pass = true;
/*@null@*/char *clear_pass = NULL;

/*
 * pw_auth - perform getpass/crypt authentication
 *
 *	pw_auth gets the user's cleartext password and encrypts it
 *	using the salt in the encrypted password. The results are
 *	compared.
 */

int pw_auth (const char *cipher,
             const char *user,
             int reason,
             /*@null@*/const char *input)
{
	char prompt[1024];
	char *clear = NULL;
	const char *cp;
	const char *encrypted;
	int retval;

#ifdef	SKEY
	bool use_skey = false;
	char challenge_info[40];
	struct skey skey;
#endif

	/*
	 * There are programs for adding and deleting authentication data.
	 */

	if ((PW_ADD == reason) || (PW_DELETE == reason)) {
		return 0;
	}

	/*
	 * There are even programs for changing the user name ...
	 */

	if ((PW_CHANGE == reason) && (NULL != input)) {
		return 0;
	}

	/*
	 * WARNING:
	 *
	 * When we change a password and we are root, we don't prompt.
	 * This is so root can change any password without having to
	 * know it.  This is a policy decision that might have to be
	 * revisited.
	 */

	if ((PW_CHANGE == reason) && (getuid () == 0)) {
		return 0;
	}

	/*
	 * WARNING:
	 *
	 * When we are logging in a user with no ciphertext password,
	 * we don't prompt for the password or anything.  In reality
	 * the user could just hit <ENTER>, so it doesn't really
	 * matter.
	 */

	if ((NULL == cipher) || ('\0' == *cipher)) {
		return 0;
	}

#ifdef	SKEY
	/*
	 * If the user has an S/KEY entry show them the pertinent info
	 * and then we can try validating the created ciphertext and the SKEY.
	 * If there is no SKEY information we default to not using SKEY.
	 */

# ifdef SKEY_BSD_STYLE
	/*
	 * Some BSD updates to the S/KEY API adds a fourth parameter; the
	 * sizeof of the challenge info buffer.
	 */
#  define skeychallenge(s,u,c) skeychallenge(s,u,c,sizeof(c))
# endif

	if (skeychallenge (&skey, user, challenge_info) == 0) {
		use_skey = true;
	}
#endif

	/*
	 * Prompt for the password as required.  FTPD and REXECD both
	 * get the cleartext password for us.
	 */

	if ((PW_FTP != reason) && (PW_REXEC != reason) && (NULL == input)) {
		cp = getdef_str ("LOGIN_STRING");
		if (NULL == cp) {
			cp = _(PROMPT);
		}
#ifdef	SKEY
		if (use_skey) {
			printf ("[%s]\n", challenge_info);
		}
#endif

		snprintf (prompt, sizeof prompt, cp, user);
		clear = getpass (prompt);
		if (NULL == clear) {
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

	encrypted = pw_encrypt (input, cipher);
	if (NULL != encrypted) {
		retval = strcmp (encrypted, cipher);
	} else {
		retval = -1;
	}

#ifdef  SKEY
	/*
	 * If (1) The password fails to match, and
	 * (2) The password is empty and
	 * (3) We are using OPIE or S/Key, then
	 * ...Re-prompt, with echo on.
	 * -- AR 8/22/1999
	 */
	if ((0 != retval) && ('\0' == input[0]) && use_skey) {
		clear = getpass (prompt);
		if (NULL == clear) {
			static char c[1];

			c[0] = '\0';
			clear = c;
		}
		input = clear;
	}

	if ((0 != retval) && use_skey) {
		int passcheck = -1;

		if (skeyverify (&skey, input) == 0) {
			passcheck = skey.n;
		}
		if (passcheck > 0) {
			retval = 0;
		}
	}
#endif

	/*
	 * Things like RADIUS authentication may need the password -
	 * if the external variable wipe_clear_pass is zero, we will
	 * not wipe it (the caller should wipe clear_pass when it is
	 * no longer needed).  --marekm
	 */

	clear_pass = clear;
	if (wipe_clear_pass && (NULL != clear)) {
		strzero (clear);
	}
	return retval;
}
#else				/* !USE_PAM */
extern int ISO_C_forbids_an_empty_translation_unit;
#endif				/* !USE_PAM */
