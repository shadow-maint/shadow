// SPDX-FileCopyrightText: 1991-1994, Julianne Frances Haugh
// SPDX-FileCopyrightText: 1996-1999, Marek Michałkiewicz
// SPDX-FileCopyrightText: 2003-2005, Tomasz Kłoczko
// SPDX-FileCopyrightText: 2008, Nicolas François
// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause

#include <config.h>

#include <ctype.h>
#include <time.h>

#ident "$Id$"

#include "prototypes.h"


long
strtoday(const char *str)
{
	bool        isnum = true;
	time_t      t;
	struct tm   *tp;
	const char  *s = str;

	if ((NULL == str) || ('\0' == *str))
		return -1;

	/* If a numerical value is provided, this is already a number of
	 * days since EPOCH.
	 */
	if ('-' == *s) {
		s++;
	}
	while (' ' == *s) {
		s++;
	}
	while (isnum && ('\0' != *s)) {
		if (!isdigit (*s)) {
			isnum = false;
		}
		s++;
	}
	if (isnum) {
		long retdate;
		if (getlong(str, &retdate) == -1) {
			return -2;
		}
		return retdate;
	}

	tp = getdate(str);
	if (tp == NULL)
		return -2;

	t = mktime(tp);
	if ((time_t) -1 == t)
		return -2;

	return t / DAY;
}
