/*
 * Copyright 1991 - 1994, Julianne Frances Haugh
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

#if !defined(__GLIBC__)
#define _XOPEN_SOURCE 500
#endif

#include <config.h>

#include "rcsid.h"
RCSID ("$Id: strtoday.c,v 1.9 2003/04/22 10:59:22 kloczek Exp $")
#include "defines.h"
#ifndef USE_GETDATE
#define USE_GETDATE 1
#endif
#if USE_GETDATE
#include "getdate.h"
/*
 * strtoday() now uses get_date() (borrowed from GNU shellutils)
 * which can handle many date formats, for example:
 *	1970-09-17	# ISO 8601.
 *	70-9-17		# This century assumed by default.
 *	70-09-17	# Leading zeros are ignored.
 *	9/17/72		# Common U.S. writing.
 *	24 September 1972
 *	24 Sept 72	# September has a special abbreviation.
 *	24 Sep 72	# Three-letter abbreviations always allowed.
 *	Sep 24, 1972
 *	24-sep-72
 *	24sep72
 */
long strtoday (const char *str)
{
	time_t t;

	/*
	 * get_date() interprets an empty string as the current date,
	 * which is not what we expect, unless you're a BOFH :-).
	 * (useradd sets sp_expire = current date for new lusers)
	 */
	if (!str || *str == '\0')
		return -1;

	t = get_date (str, (time_t *) 0);
	if (t == (time_t) - 1)
		return -1;
	/* convert seconds to days since 1970-01-01 */
	return (t + DAY / 2) / DAY;
}

#else				/* !USE_GETDATE */
/*
 * Old code, just in case get_date() doesn't work as expected...
 */
#include <stdio.h>
#ifdef HAVE_STRPTIME
/*
 * for now we allow just one format, but we can define more later
 * (we try them all until one succeeds).  --marekm
 */
static char *date_formats[] = {
	"%Y-%m-%d",
	(char *) 0
};
#else
/*
 * days and juldays are used to compute the number of days in the
 * current month, and the cummulative number of days in the preceding
 * months.  they are declared so that january is 1, not 0.
 */
static short days[13] = { 0,
	31, 28, 31, 30, 31, 30,	/* JAN - JUN */
	31, 31, 30, 31, 30, 31
};				/* JUL - DEC */

static short juldays[13] = { 0,
	0, 31, 59, 90, 120, 151,	/* JAN - JUN */
	181, 212, 243, 273, 304, 334
};				/* JUL - DEC */
#endif

/*
 * strtoday - compute the number of days since 1970.
 *
 * the total number of days prior to the current date is
 * computed.  january 1, 1970 is used as the origin with
 * it having a day number of 0.
 */

long strtoday (const char *str)
{
#ifdef HAVE_STRPTIME
	struct tm tp;
	char *const *fmt;
	char *cp;
	time_t result;

	memzero (&tp, sizeof tp);
	for (fmt = date_formats; *fmt; fmt++) {
		cp = strptime ((char *) str, *fmt, &tp);
		if (!cp || *cp != '\0')
			continue;

		result = mktime (&tp);
		if (result == (time_t) - 1)
			continue;

		return result / DAY;	/* success */
	}
	return -1;
#else
	char slop[2];
	int month;
	int day;
	int year;
	long total;

	/*
	 * start by separating the month, day and year.  the order
	 * is compiled in ...
	 */

	if (sscanf (str, "%d/%d/%d%c", &year, &month, &day, slop) != 3)
		return -1;

	/*
	 * the month, day of the month, and year are checked for
	 * correctness and the year adjusted so it falls between
	 * 1970 and 2069.
	 */

	if (month < 1 || month > 12)
		return -1;

	if (day < 1)
		return -1;

	if ((month != 2 || (year % 4) != 0) && day > days[month])
		return -1;
	else if ((month == 2 && (year % 4) == 0) && day > 29)
		return -1;

	if (year < 0)
		return -1;
	else if (year <= 69)
		year += 2000;
	else if (year <= 99)
		year += 1900;

	/*
	 * On systems with 32-bit signed time_t, time wraps around in 2038
	 * - for now we just limit the year to 2037 (instead of 2069).
	 * This limit can be removed once no one is using 32-bit systems
	 * anymore :-).  --marekm
	 */
	if (year < 1970 || year > 2037)
		return -1;

	/*
	 * the total number of days is the total number of days in all
	 * the whole years, plus the number of leap days, plus the
	 * number of days in the whole months preceding, plus the number
	 * of days so far in the month.
	 */

	total = (long) ((year - 1970) * 365L) + (((year + 1) - 1970) / 4);
	total += (long) juldays[month] + (month > 2
					  && (year % 4) == 0 ? 1 : 0);
	total += (long) day - 1;

	return total;
#endif				/* HAVE_STRPTIME */
}
#endif				/* !USE_GETDATE */
