// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_TIME_DATE_H_
#define SHADOW_INCLUDE_LIB_TIME_DATE_H_


#include "config.h"

#include <time.h>

#include "defines.h"


inline long date_or_SDE(void);
time_t gettime(void);


// Like time_or_SDE(), but return a date, not a time.
inline long
date_or_SDE(void)
{
	return gettime() / DAY;
}


#endif  // include guard
