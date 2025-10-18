// SPDX-FileCopyrightText: 2023-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ATOI_A2I_H_
#define SHADOW_INCLUDE_LIB_ATOI_A2I_H_


#include "config.h"

#include <errno.h>
#include <stddef.h>

#include "atoi/strtoi/strtoi.h"
#include "atoi/strtoi/strtou_noneg.h"
#include "typetraits.h"


/*
 * See the manual of these macros in liba2i's documentation:
 * <http://www.alejandro-colomar.es/share/dist/liba2i/git/HEAD/liba2i-HEAD.pdf>
 */


// QChar_of - possibly-qualified char of-a-string
#define QChar_of(s)  typeof                                           \
(                                                                     \
	_Generic(s,                                                   \
		const char *:  (const char){1},                       \
		const void *:  (const char){1},                       \
		char *:        (char){1},                             \
		void *:        (char){1}                              \
	)                                                             \
)


#define a2i_(T, QChar, n, s, endp, base, min, max)                    \
({                                                                    \
	T      *n_ = n;                                               \
	QChar  **endp_ = endp;                                        \
	T      min_ = min;                                            \
	T      max_ = max;                                            \
                                                                      \
	int  status;                                                  \
                                                                      \
	*n_ = _Generic((T) 0,                                         \
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


// a2i - alpha to integer
#define a2i(T, n, s, ...)  a2i_(T, QChar_of(s), n, s, __VA_ARGS__)

#define a2sh(...)   a2i(short, __VA_ARGS__)
#define a2si(...)   a2i(int, __VA_ARGS__)
#define a2sl(...)   a2i(long, __VA_ARGS__)
#define a2sll(...)  a2i(long long, __VA_ARGS__)

#define a2uh(...)   a2i(unsigned short, __VA_ARGS__)
#define a2ui(...)   a2i(unsigned int, __VA_ARGS__)
#define a2ul(...)   a2i(unsigned long, __VA_ARGS__)
#define a2ull(...)  a2i(unsigned long long, __VA_ARGS__)

#define str2i(T, ...)  a2i(T, __VA_ARGS__, NULL, 0, type_min(T), type_max(T))

#define str2sh(...)    str2i(short, __VA_ARGS__)
#define str2si(...)    str2i(int, __VA_ARGS__)
#define str2sl(...)    str2i(long, __VA_ARGS__)
#define str2sll(...)   str2i(long long, __VA_ARGS__)

#define str2uh(...)    str2i(unsigned short, __VA_ARGS__)
#define str2ui(...)    str2i(unsigned int, __VA_ARGS__)
#define str2ul(...)    str2i(unsigned long, __VA_ARGS__)
#define str2ull(...)   str2i(unsigned long long, __VA_ARGS__)


#endif  // include guard
