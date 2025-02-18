/*
 * SPDX-FileCopyrightText: 1991 - 1994, Julianne Frances Haugh
 * SPDX-FileCopyrightText: 1996 - 1999, Marek Michałkiewicz
 * SPDX-FileCopyrightText: 2003 - 2005, Tomasz Kłoczko
 * SPDX-FileCopyrightText: 2008       , Nicolas François
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <config.h>

#include <ctype.h>

#include "atoi/str2i.h"
#include "getdate.h"
#include "prototypes.h"
#include "string/strcmp/streq.h"


// string parse-to day-since-Epoch
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
