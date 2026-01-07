// SPDX-FileCopyrightText: 1989-1994, Julianne Frances Haugh
// SPDX-FileCopyrightText: 1996-1999, Marek Michałkiewicz
// SPDX-FileCopyrightText: 2003-2005, Tomasz Kłoczko
// SPDX-FileCopyrightText: 2007-2010, Nicolas François
// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#ident "$Id$"


#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "attr.h"
#include "prototypes.h"
#include "defines.h"
#include "getdef.h"
#include "string/ctype/strtoascii/strtolower.h"
#include "string/memset/memzero.h"
#include "string/sprintf/aprintf.h"
#include "string/strcmp/streq.h"
#include "string/strdup/strdup.h"


#if WITH_LIBBSD == 0
#include "freezero.h"
#endif /* WITH_LIBBSD */

/*
 * can't be a palindrome - like `R A D A R' or `M A D A M'
 */
static bool
palindrome(const char *new)
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
		if (strchr(new, old[i]))
			j++;
	}

	if (i >= j * 2) {
		return false;
	}

	return true;
}


static /*@observer@*//*@null@*/const char *password_check (
	/*@notnull@*/const char *old,
	/*@notnull@*/const char *new)
{
	const char *msg = NULL;
	char *oldmono, *newmono, *wrapped;

	if (streq(new, old)) {
		return _("no change");
	}

	newmono = strtolower(xstrdup(new));
	oldmono = strtolower(xstrdup(old));
	wrapped = xaprintf("%s%s", oldmono, oldmono);

	if (palindrome(newmono)) {
		msg = _("a palindrome");
	} else if (streq(oldmono, newmono)) {
		msg = _("case changes only");
	} else if (similar (oldmono, newmono)) {
		msg = _("too similar");
	} else if (strstr (wrapped, newmono) != NULL) {
		msg = _("rotated");
	}
	free(strzero(newmono));
	free(strzero(oldmono));
	free(strzero(wrapped));

	return msg;
}

static /*@observer@*//*@null@*/const char *obscure_msg (
	/*@notnull@*/const char *old,
	/*@notnull@*/const char *new)
{
	int maxlen, minlen;
	size_t oldlen, newlen;
	char *new1, *old1;
	const char *msg;

	oldlen = strlen (old);
	newlen = strlen (new);

	obscure_get_range(&minlen, &maxlen);

	if (newlen < (size_t) minlen) {
		return _("too short");
	}

	/*
	 * Remaining checks are optional.
	 */
	if (!getdef_bool ("OBSCURE_CHECKS_ENAB")) {
		return NULL;
	}

	msg = password_check(old, new);
	if (NULL != msg) {
		return msg;
	}

	if (maxlen == -1) {
		return NULL;
	}

	/* The traditional crypt() truncates passwords to 8 chars.  It is
	   possible to circumvent the above checks by choosing an easy
	   8-char password and adding some random characters to it...
	   Example: "password$%^&*123".  So check it again, this time
	   truncated to the maximum length.  Idea from npasswd.  --marekm */

	if (   (oldlen <= (size_t) maxlen)
	    && (newlen <= (size_t) maxlen)) {
		return NULL;
	}

	new1 = xstrdup (new);
	old1 = xstrdup (old);
	if (newlen > (size_t) maxlen)
		stpcpy(&new1[maxlen], "");
	if (oldlen > (size_t) maxlen)
		stpcpy(&old1[maxlen], "");

	msg = password_check(old1, new1);

	freezero (new1, newlen);
	freezero (old1, oldlen);

	return msg;
}

/*
 * Obscure - see if password is obscure enough.
 *
 *	The programmer is encouraged to add as much complexity to this
 *	routine as desired.  Included are some of my favorite ways to
 *	check passwords.
 */

bool
obscure(const char *old, const char *new)
{
	const char *msg = obscure_msg(old, new);

	if (NULL != msg) {
		printf (_("Bad password: %s.  "), msg);
		return false;
	}
	return true;
}

/*
 * obscure_get_range - retrieve min and max password lengths
 *
 *  Returns minimum and maximum allowed lengths of a password
 *  to pass obscure checks.
 */
void
obscure_get_range(int *minlen, int *maxlen)
{
	int        val;
	const char *method;

	/* Minimum length is 0, even if -1 is configured. */
	val = getdef_num("PASS_MIN_LEN", 0);
	*minlen = val == -1 ? 0 : val;

	/* Maximum password length check is optional. */
	*maxlen = -1;

	if (!getdef_bool("OBSCURE_CHECKS_ENAB")) {
		return;
	}

	method = getdef_str ("ENCRYPT_METHOD");
	if (NULL == method)
		return;

	if (!streq(method, "DES"))
		return;

	*maxlen = getdef_num ("PASS_MAX_LEN", 8);
}
