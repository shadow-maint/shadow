// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-FileCopyrightText: 2025, "Haelwenn (lanodan) Monnier" <contact@hacktivis.me>
// SPDX-License-Identifier: BSD-3-Clause


#include <config.h>

#include <stddef.h>
#include <time.h>

#include "getdate.h"
#include "string/strcmp/streq.h"


time_t
get_date(const char *s)
{
	time_t      t;
	struct tm   tm;
	const char  *p;

	t = 0;
	if (gmtime_r(&t, &tm) == NULL)
		return -1;

	p = strptime(s, "%Y-%m-%d", &tm);
	if (p == NULL || !streq(p, ""))
		return -1;

	return timegm(&tm);
}
