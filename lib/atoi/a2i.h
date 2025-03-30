// SPDX-FileCopyrightText: 2023-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ATOI_A2I_H_
#define SHADOW_INCLUDE_LIB_ATOI_A2I_H_


#include <config.h>

#include <errno.h>
#include <stdint.h>
#include <sys/types.h>

#include "atoi/strton.h"
#include "attr.h"


/*
 * See the manual of these macros in liba2i's documentation:
 * <http://www.alejandro-colomar.es/share/dist/liba2i/git/HEAD/liba2i-HEAD.pdf>
 */
#define a2i(T, n, s, ...)                                             \
(                                                                     \
	_Generic(s,                                                   \
		const char *:  A2I_c(T),                              \
		const void *:  A2I_c(T),                              \
		char *:        A2I_nc(T),                             \
		void *:        A2I_nc(T)                              \
	)(n, s, __VA_ARGS__)                                          \
)

#define a2sh(...)   a2i(short, __VA_ARGS__)
#define a2si(...)   a2i(int, __VA_ARGS__)
#define a2sl(...)   a2i(long, __VA_ARGS__)
#define a2sll(...)  a2i(long long, __VA_ARGS__)

#define a2uh(...)   a2i(unsigned short, __VA_ARGS__)
#define a2ui(...)   a2i(unsigned int, __VA_ARGS__)
#define a2ul(...)   a2i(unsigned long, __VA_ARGS__)
#define a2ull(...)  a2i(unsigned long long, __VA_ARGS__)


typedef long long           llong;
typedef unsigned long long  u_llong;


#define A2I_nc(T)                                                     \
(                                                                     \
	_Generic((T) 0,                                               \
		short:    A2I_nc__ ## short,                          \
		int:      A2I_nc__ ## int,                            \
		long:     A2I_nc__ ## long,                           \
		llong:    A2I_nc__ ## llong,                          \
		u_short:  A2I_nc__ ## u_short,                        \
		u_int:    A2I_nc__ ## u_int,                          \
		u_long:   A2I_nc__ ## u_long,                         \
		u_llong:  A2I_nc__ ## u_llong                         \
	)                                                             \
)

#define template_A2I_nc(T)                                            \
ATTR_STRING(2) ATTR_ACCESS(write_only, 1) ATTR_ACCESS(write_only, 3)  \
inline int                                                            \
A2I_nc__ ## T(T *restrict n, char *s,                                 \
    char **restrict endp, int base, T min, T max)                     \
{                                                                     \
	int  status;                                                  \
                                                                      \
	*n = _Generic((T) 0,                                          \
		short:    STRTON(intmax_t),                           \
		int:      STRTON(intmax_t),                           \
		long:     STRTON(intmax_t),                           \
		llong:    STRTON(intmax_t),                           \
		u_short:  strtou_noneg,                               \
		u_int:    strtou_noneg,                               \
		u_long:   strtou_noneg,                               \
		u_llong:  strtou_noneg                                \
	)(s, endp, base, min, max, &status);                          \
	if (status != 0) {                                            \
		errno = status;                                       \
		return -1;                                            \
	}                                                             \
	return 0;                                                     \
}
template_A2I_nc(short);
template_A2I_nc(int);
template_A2I_nc(long);
template_A2I_nc(llong);
template_A2I_nc(u_short);
template_A2I_nc(u_int);
template_A2I_nc(u_long);
template_A2I_nc(u_llong);
#undef template_A2I_nc


#define A2I_c(T)                                                      \
(                                                                     \
	_Generic((T) 0,                                               \
		short:    A2I_c__ ## short,                           \
		int:      A2I_c__ ## int,                             \
		long:     A2I_c__ ## long,                            \
		llong:    A2I_c__ ## llong,                           \
		u_short:  A2I_c__ ## u_short,                         \
		u_int:    A2I_c__ ## u_int,                           \
		u_long:   A2I_c__ ## u_long,                          \
		u_llong:  A2I_c__ ## u_llong                          \
	)                                                             \
)

#define template_A2I_c(T)                                             \
ATTR_STRING(2) ATTR_ACCESS(write_only, 1) ATTR_ACCESS(write_only, 3)  \
inline int                                                            \
A2I_c__ ## T(T *restrict n, const char *s,                            \
    const char **restrict endp, int base, T min, T max)               \
{                                                                     \
	return A2I_nc(T)(n, (char *) s, (char **) endp, base, min, max); \
}
template_A2I_c(short);
template_A2I_c(int);
template_A2I_c(long);
template_A2I_c(llong);
template_A2I_c(u_short);
template_A2I_c(u_int);
template_A2I_c(u_long);
template_A2I_c(u_llong);
#undef template_A2I_c


#endif  // include guard
