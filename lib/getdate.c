// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include <config.h>

#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <string.h>
#include <time.h>

#include "atoi/a2i/a2s.h"
#include "getdate.h"
#include "string/strchr/strchrcnt.h"
#include "string/strcmp/streq.h"
#include "string/strcmp/strprefix.h"
#include "string/strcmp/strsuffix.h"
#include "string/strspn/stpspn.h"


#define TM_YEAR_ORIGIN 1900


static long  yyDay;
static long  yyMonth;
static long  yyYear;


static int parse_date(const char *s);


time_t
get_date(const char *s)
{
	struct tm  tm;

	if (parse_date(s) == -1)
		return -1;

	tm.tm_year = yyYear - TM_YEAR_ORIGIN;
	tm.tm_mon = yyMonth - 1;
	tm.tm_mday = yyDay;
	tm.tm_hour = tm.tm_min = tm.tm_sec = 0;
	tm.tm_isdst = 0;

	return timegm(&tm);
}


static int
parse_date(const char *s)
{
	long  n;

	if (!streq(stpspn(s, "0123456789-"), "")
	 || strchrcnt(s, '-') != 2
	 || strprefix(s, "-")
	 || strsuffix(s, "-")
	 || strstr(s, "--"))
	{
		return -1;
	}

	if (a2sl(&yyYear, s, &s, 10, LONG_MIN, LONG_MAX) == -1 && errno != ENOTSUP)
		return -1;

	if (!strprefix(s++, "-"))
		return -1;

	if (a2sl(&yyMonth, s, &s, 10, LONG_MIN, LONG_MAX) == -1 && errno != ENOTSUP)
		return -1;

	if (!strprefix(s++, "-"))
		return -1;

	if (a2sl(&yyDay, s, NULL, 10, LONG_MIN, LONG_MAX) == -1)
		return -1;

	return 0;
}
