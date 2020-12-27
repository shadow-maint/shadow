/*
 * Copyright (c) 1989 - 1994, Julianne Frances Haugh
 * Copyright (c) 1996 - 1999, Marek Michałkiewicz
 * Copyright (c) 2003 - 2005, Tomasz Kłoczko
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

#include <config.h>

#ifndef USE_PAM

#ident "$Id$"


/*
 * This version of obscure.c contains modifications to support "cracklib"
 * by Alec Muffet (alec.muffett@uk.sun.com).  You must obtain the Cracklib
 * library source code for this function to operate.
 */
#include <ctype.h>
#include <stdio.h>
#include "prototypes.h"
#include "defines.h"
#include "getdef.h"
/*
 * can't be a palindrome - like `R A D A R' or `M A D A M'
 */
static bool palindrome (unused const char *old, const char *new)
{
	size_t i, j;

	i = strlen (new);

	for (j = 0; j < i; j++) {
		if (new[i - j - 1] != new[j]) {
			return false;
		}
	}

	return true;
}

/*
 * more than half of the characters are different ones.
 */

static bool similar (/*@notnull@*/const char *old, /*@notnull@*/const char *new)
{
	int i, j;

	/*
	 * XXX - sometimes this fails when changing from a simple password
	 * to a really long one (MD5).  For now, I just return success if
	 * the new password is long enough.  Please feel free to suggest
	 * something better...  --marekm
	 */
	if (strlen (new) >= 8) {
		return false;
	}

	for (i = j = 0; ('\0' != new[i]) && ('\0' != old[i]); i++) {
		if (strchr (new, old[i]) != NULL) {
			j++;
		}
	}

	if (i >= j * 2) {
		return false;
	}

	return true;
}

/*
 * a nice mix of characters.
 */

static bool simple (unused const char *old, const char *new)
{
	bool digits = false;
	bool uppers = false;
	bool lowers = false;
	bool others = false;
	int size;
	int i;

	for (i = 0; '\0' != new[i]; i++) {
		if (isdigit (new[i])) {
			digits = true;
		} else if (isupper (new[i])) {
			uppers = true;
		} else if (islower (new[i])) {
			lowers = true;
		} else {
			others = true;
		}
	}

	/*
	 * The scam is this - a password of only one character type
	 * must be 8 letters long.  Two types, 7, and so on.
	 */

	size = 9;
	if (digits) {
		size--;
	}
	if (uppers) {
		size--;
	}
	if (lowers) {
		size--;
	}
	if (others) {
		size--;
	}

	if (size <= i) {
		return false;
	}

	return true;
}

static char *str_lower (/*@returned@*/char *string)
{
	char *cp;

	for (cp = string; '\0' != *cp; cp++) {
		*cp = tolower (*cp);
	}
	return string;
}

static /*@observer@*//*@null@*/const char *password_check (
	/*@notnull@*/const char *old,
	/*@notnull@*/const char *new,
	/*@notnull@*/const struct passwd *pwdp)
{
	const char *msg = NULL;
	char *oldmono, *newmono, *wrapped;

#ifdef HAVE_LIBCRACK
	char *dictpath;

#ifdef HAVE_LIBCRACK_PW
	char *FascistCheckPw ();
#else
	char *FascistCheck ();
#endif
#endif

	if (strcmp (new, old) == 0) {
		return _("no change");
	}

	newmono = str_lower (xstrdup (new));
	oldmono = str_lower (xstrdup (old));
	wrapped = xmalloc (strlen (oldmono) * 2 + 1);
	strcpy (wrapped, oldmono);
	strcat (wrapped, oldmono);

	if (palindrome (oldmono, newmono)) {
		msg = _("a palindrome");
	} else if (strcmp (oldmono, newmono) == 0) {
		msg = _("case changes only");
	} else if (similar (oldmono, newmono)) {
		msg = _("too similar");
	} else if (simple (old, new)) {
		msg = _("too simple");
	} else if (strstr (wrapped, newmono) != NULL) {
		msg = _("rotated");
	} else {
#ifdef HAVE_LIBCRACK
		/*
		 * Invoke Alec Muffett's cracklib routines.
		 */

		dictpath = getdef_str ("CRACKLIB_DICTPATH");
		if (NULL != dictpath) {
#ifdef HAVE_LIBCRACK_PW
			msg = FascistCheckPw (new, dictpath, pwdp);
#else
			msg = FascistCheck (new, dictpath);
#endif
		}
#endif
	}
	strzero (newmono);
	strzero (oldmono);
	strzero (wrapped);
	free (newmono);
	free (oldmono);
	free (wrapped);

	return msg;
}

static /*@observer@*//*@null@*/const char *obscure_msg (
	/*@notnull@*/const char *old,
	/*@notnull@*/const char *new,
	/*@notnull@*/const struct passwd *pwdp)
{
	size_t maxlen, oldlen, newlen;
	char *new1, *old1;
	const char *msg;
	const char *result;

	oldlen = strlen (old);
	newlen = strlen (new);

	if (newlen < (size_t) getdef_num ("PASS_MIN_LEN", 0)) {
		return _("too short");
	}

	/*
	 * Remaining checks are optional.
	 */
	if (!getdef_bool ("OBSCURE_CHECKS_ENAB")) {
		return NULL;
	}

	msg = password_check (old, new, pwdp);
	if (NULL != msg) {
		return msg;
	}

	result = getdef_str ("ENCRYPT_METHOD");
	if (NULL == result) {
	/* The traditional crypt() truncates passwords to 8 chars.  It is
	   possible to circumvent the above checks by choosing an easy
	   8-char password and adding some random characters to it...
	   Example: "password$%^&*123".  So check it again, this time
	   truncated to the maximum length.  Idea from npasswd.  --marekm */

		if (getdef_bool ("MD5_CRYPT_ENAB")) {
			return NULL;
		}

	} else {

		if (   (strcmp (result, "MD5")    == 0)
#ifdef USE_SHA_CRYPT
		    || (strcmp (result, "SHA256") == 0)
		    || (strcmp (result, "SHA512") == 0)
#endif
#ifdef USE_BCRYPT
		    || (strcmp (result, "BCRYPT") == 0)
#endif
#ifdef USE_YESCRYPT
		    || (strcmp (result, "YESCRYPT") == 0)
#endif
		    ) {
			return NULL;
		}

	}
	maxlen = (size_t) getdef_num ("PASS_MAX_LEN", 8);
	if (   (oldlen <= maxlen)
	    && (newlen <= maxlen)) {
		return NULL;
	}

	new1 = xstrdup (new);
	old1 = xstrdup (old);
	if (newlen > maxlen) {
		new1[maxlen] = '\0';
	}
	if (oldlen > maxlen) {
		old1[maxlen] = '\0';
	}

	msg = password_check (old1, new1, pwdp);

	memzero (new1, newlen);
	memzero (old1, oldlen);
	free (new1);
	free (old1);

	return msg;
}

/*
 * Obscure - see if password is obscure enough.
 *
 *	The programmer is encouraged to add as much complexity to this
 *	routine as desired.  Included are some of my favorite ways to
 *	check passwords.
 */

bool obscure (const char *old, const char *new, const struct passwd *pwdp)
{
	const char *msg = obscure_msg (old, new, pwdp);

	if (NULL != msg) {
		printf (_("Bad password: %s.  "), msg);
		return false;
	}
	return true;
}

#else				/* !USE_PAM */
extern int errno;		/* warning: ANSI C forbids an empty source file */
#endif				/* !USE_PAM */
