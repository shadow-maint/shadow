// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include <config.h>

#include <stddef.h>
#include <strings.h>
#include <time.h>

#include "getdate.h"
#include "string/strcmp/streq.h"


time_t
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
