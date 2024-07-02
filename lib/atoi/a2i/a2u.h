// SPDX-FileCopyrightText: 2023-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ATOI_A2I_A2U_H_
#define SHADOW_INCLUDE_LIB_ATOI_A2I_A2U_H_


#include <config.h>

#include "atoi/a2i/a2u_c.h"
#include "atoi/a2i/a2u_nc.h"


#define a2uh(n, s, ...)                                                       \
(                                                                             \
	_Generic(s,                                                           \
		const char *:  a2uh_c,                                        \
		const void *:  a2uh_c,                                        \
		char *:        a2uh_nc,                                       \
		void *:        a2uh_nc                                        \
	)(n, s, __VA_ARGS__)                                                  \
)

#define a2ui(n, s, ...)                                                       \
(                                                                             \
	_Generic(s,                                                           \
		const char *:  a2ui_c,                                        \
		const void *:  a2ui_c,                                        \
		char *:        a2ui_nc,                                       \
		void *:        a2ui_nc                                        \
	)(n, s, __VA_ARGS__)                                                  \
)

#define a2ul(n, s, ...)                                                       \
(                                                                             \
	_Generic(s,                                                           \
		const char *:  a2ul_c,                                        \
		const void *:  a2ul_c,                                        \
		char *:        a2ul_nc,                                       \
		void *:        a2ul_nc                                        \
	)(n, s, __VA_ARGS__)                                                  \
)

#define a2ull(n, s, ...)                                                      \
(                                                                             \
	_Generic(s,                                                           \
		const char *:  a2ull_c,                                       \
		const void *:  a2ull_c,                                       \
		char *:        a2ull_nc,                                      \
		void *:        a2ull_nc                                       \
	)(n, s, __VA_ARGS__)                                                  \
)


#endif  // include guard
