// SPDX-FileCopyrightText: 1991-1994, Julianne Frances Haugh
// SPDX-FileCopyrightText: 1996-1999, Marek Michałkiewicz
// SPDX-FileCopyrightText: 2003-2005, Tomasz Kłoczko
// SPDX-FileCopyrightText: 2008, Nicolas François
// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include <config.h>

#include <stddef.h>
#include <strings.h>
#include <time.h>

#include "atoi/str2i/str2s.h"
#include "defines.h"
#include "prototypes.h"
#include "string/strcmp/streq.h"


static time_t get_date(const char *s);


long
strtoday(const char *str)
{
	long  d;
	time_t t;

	if (NULL == str || streq(str, ""))
		return -1;

	/* If a numerical value is provided, this is already a number of
	 * days since EPOCH.
	 */
	if (str2sl(&d, str) == 0)
		return d;

	t = get_date(str);
	if ((time_t) - 1 == t) {
		return -2;
	}
	return t / DAY;
}


static time_t
get_date(const char *s)
{
	struct tm   tm;
	const char  *p;

	bzero(&tm, sizeof(tm));

	p = strptime("UTC", "%Z", &tm);
	if (p == NULL || !streq(p, ""))
		return -1;

	p = strptime(s, "%F", &tm);
	if (p == NULL || !streq(p, ""))
		return -1;

	return timegm(&tm);
}
