/*
 * SPDX-FileCopyrightText: 2021-2023, Alejandro Colomar <alx@kernel.org>
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef SHADOW_INCLUDE_LIB_TIME_DAY_TO_STR_H_
#define SHADOW_INCLUDE_LIB_TIME_DAY_TO_STR_H_


#include <config.h>

#include <time.h>

#include "defines.h"
#include "sizeof.h"
#include "string/strtcpy.h"


#define DAY_TO_STR(str, day)   date_to_str(NITEMS(str), str, day * DAY)


inline void date_to_str(size_t size, char buf[size], long date);


inline void
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


#endif  // include guard
