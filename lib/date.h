// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_DATE_H_
#define SHADOW_INCLUDE_LIB_DATE_H_


#include "config.h"

#include <time.h>

#include "defines.h"


inline long date(void);
time_t gettime(void);


inline long
date(void)
{
	time_t  t;

	t = gettime();
	if (t == -1)
		return -1;

	return t / DAY;
}


#endif  // include guard
