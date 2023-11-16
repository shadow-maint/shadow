/*
 * SPDX-FileCopyrightText: 2021-2023, Alejandro Colomar <alx@kernel.org>
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <config.h>

#include <string.h>
#include <time.h>

#ident "$Id$"

#include "strtcpy.h"
#include "prototypes.h"


void
date_to_str(size_t size, char buf[size], long date)
{
	time_t           t;
	const struct tm  *tm;

	t = date;
	if (date < 0) {
		(void) strtcpy(buf, "never", size);
		return;
	}

	tm = gmtime(&t);
	if (tm == NULL) {
		(void) strtcpy(buf, "future", size);
		return;
	}

	if (strftime(buf, size, "%Y-%m-%d", tm) == 0)
		(void) strtcpy(buf, "future", size);
}
