// SPDX-FileCopyrightText: 1990-1994, Julianne Frances Haugh
// SPDX-FileCopyrightText: 1996-2000, Marek Michałkiewicz
// SPDX-FileCopyrightText: 2001-2005, Tomasz Kłoczko
// SPDX-FileCopyrightText: 2005-2008, Nicolas François
// SPDX-FileCopyrightText: 2023-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


/*
 * check user/group names for valid syntax
 * return values:
 *   true  - OK
 *   false - bad name
 * errors:
 *   EINVAL	Invalid name
 *   EILSEQ	Invalid name character sequence (acceptable with --badname)
 *   EOVERFLOW	Name longer than maximum size
 */


#include "config.h"

#ident "$Id$"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>

#include "defines.h"
#include "chkname.h"
#include "sizeof.h"
#include "string/ctype/isascii.h"
#include "string/strcmp/streq.h"
#include "string/strcmp/strcaseeq.h"
#include "string/strspn/strrcspn.h"
#include "string/strtok/stpsep.h"
#include "sysconf.h"


#define DOMAIN_MAXLEN	255
#define LABEL_MAXLEN	63


int allow_bad_names = false;


static bool
is_valid_name(const char *name)
{
	if (streq(name, "")
	 || streq(name, ".")
	 || streq(name, "..")
	 || strspn(name, "-")
	 || strpbrk(name, " \"#',/:;")
	 || strchriscntrl_c(name)
	 || strisdigit_c(name))
	{
		errno = EINVAL;
		return false;
	}

	if (allow_bad_names) {
		return true;
	}

	/*
	 * User/group names must match BRE regex:
	 *    [a-zA-Z0-9_.][a-zA-Z0-9_.-]*$\?
	 *
	 * as a non-POSIX, extension, allow "$" as the last char for
	 * sake of Samba 3.x "add machine script"
	 */

	if (!(isalnum_c(*name) ||
	      *name == '_' ||
	      *name == '.'))
	{
		errno = EILSEQ;
		return false;
	}

	while (!streq(++name, "")) {
		if (!(isalnum_c(*name) ||
		      *name == '_' ||
		      *name == '.' ||
		      *name == '-' ||
		      streq(name, "$")
		     ))
		{
			errno = EILSEQ;
			return false;
		}
	}

	return true;
}


bool
is_valid_user_name(const char *name)
{
	if (strlen(name) >= LOGIN_NAME_MAX) {
		errno = EOVERFLOW;
		return false;
	}

	return is_valid_name(name);
}


bool
is_valid_group_name(const char *name)
{
	/*
	 * Arbitrary limit for group names.
	 * HP-UX 10 limits to 16 characters
	 */
	if (   (GROUP_NAME_MAX_LENGTH > 0)
	    && (strlen (name) > GROUP_NAME_MAX_LENGTH))
	{
		errno = EOVERFLOW;
		return false;
	}

	return is_valid_name (name);
}


// Validate a single DNS domain label according to RFC 1035 2.3.1.
static bool
is_valid_domain_label(const char *label)
{
	if (strlen(label) > LABEL_MAXLEN) {
		errno = EOVERFLOW;
		return false;
	}
	if (!strspn(label, CTYPE_ALPHA_C) || !strrcspn(label, "-")) {
		errno = EINVAL;
		return false;
	}
	if (!streq(stpspn(label, CTYPE_ALNUM_C "-"), "")) {
		errno = EINVAL;
		return false;
	}

	return true;
}


/*
 * Validate a domain name according to RFC 1035 by splitting
 * it into labels and validating each label individually.
 */
static bool
is_valid_domain_name(const char *domain)
{
	char        *d;
	const char  *l;

	if (!strcspn(domain, ".")) {
		errno = EINVAL;
		return false;
	}

	if (strlen(domain) + !!strrcspn(domain, ".") > DOMAIN_MAXLEN) {
		errno = EOVERFLOW;
		return false;
	}
	d = strdupa(domain);

	while (NULL != (l = strsep(&d, "."))) {
		if (d == NULL && streq(l, ""))  // trailing root label
			break;
		if (!is_valid_domain_label(l))
			return false;
	}

	return true;
}


/*
 * is_valid_upn - is valid User Principal Name
 *
 * Check UPN format (user@domain) for validity.
 *
 * This function only validates syntax, not whether the UPN exists
 * in any authentication system.
 */
bool
is_valid_upn(const char *upn)
{
	char  *u, *d;

	if (strlen(upn) >= LOGIN_NAME_MAX + STRLEN("@") + DOMAIN_MAXLEN) {
		errno = EOVERFLOW;
		return false;
	}
	u = strdupa(upn);

	d = stpsep(u, "@");
	if (d == NULL) {
		errno = EINVAL;
		return false;
	}

	return is_valid_user_name(u) && is_valid_domain_name(d);
}
