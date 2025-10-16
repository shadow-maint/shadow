// SPDX-FileCopyrightText: 2023-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ATOI_A2I_A2I_H_
#define SHADOW_INCLUDE_LIB_ATOI_A2I_A2I_H_


#include "config.h"

#include <errno.h>

#include "atoi/strtoi/strtoi.h"
#include "atoi/strtoi/strtou_noneg.h"
#include "typetraits.h"


// a2i - alpha to integer
#define a2i(T, n, s, endp, base, min, max)                            \
({                                                                    \
	T            *n_ = n;                                         \
	QChar_of(s)  **endp_ = endp;                                  \
	T            min_ = min;                                      \
	T            max_ = max;                                      \
                                                                      \
	int  status;                                                  \
                                                                      \
	*n_ = _Generic((T){},                                         \
		short:              strtoi_,                          \
		int:                strtoi_,                          \
		long:               strtoi_,                          \
		long long:          strtoi_,                          \
		unsigned short:     strtou_noneg,                     \
		unsigned int:       strtou_noneg,                     \
		unsigned long:      strtou_noneg,                     \
		unsigned long long: strtou_noneg                      \
	)(s, (char **) endp_, base, min_, max_, &status);             \
                                                                      \
	if (status != 0)                                              \
		errno = status;                                       \
	-!!status;                                                    \
})


#endif  // include guard
