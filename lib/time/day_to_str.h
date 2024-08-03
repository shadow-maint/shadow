// SPDX-FileCopyrightText: 2021-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-FileCopyrightText: 2024, Tobias Stoeckmann <tobias@stoeckmann.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_TIME_DAY_TO_STR_H_
#define SHADOW_INCLUDE_LIB_TIME_DAY_TO_STR_H_


#include <config.h>

#include <stdbool.h>
#include <stddef.h>
#include <time.h>

#include "defines.h"
#include "sizeof.h"
#include "string/strcpy/strtcpy.h"


#define DAY_TO_STR(str, day, iso)   day_to_str(NITEMS(str), str, day, iso)


inline void day_to_str(size_t size, char buf[size], long day, bool iso);


inline void
day_to_str(size_t size, char buf[size], long day, bool iso)
{
	time_t     date;
	struct tm  tm;

	if (day < 0) {
		strtcpy(buf, "never", size);
		return;
	}

	if (__builtin_mul_overflow(day, DAY, &date)) {
		strtcpy(buf, "future", size);
		return;
	}

	if (gmtime_r(&date, &tm) == NULL) {
		strtcpy(buf, "future", size);
		return;
	}

	if (strftime(buf, size, iso ? "%F" : "%x", &tm) == 0)
		strtcpy(buf, "future", size);
}


#endif  // include guard
