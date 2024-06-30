// SPDX-FileCopyrightText: 2023-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ATOI_A2I_A2S_H_
#define SHADOW_INCLUDE_LIB_ATOI_A2I_A2S_H_


#include <config.h>

#include "atoi/a2i/a2s_c.h"
#include "atoi/a2i/a2s_nc.h"


#define a2sh(n, s, ...)                                                       \
(                                                                             \
	_Generic(s,                                                           \
		const char *:  a2sh_c,                                        \
		const void *:  a2sh_c,                                        \
		char *:        a2sh_nc,                                       \
		void *:        a2sh_nc                                        \
	)(n, s, __VA_ARGS__)                                                  \
)

#define a2si(n, s, ...)                                                       \
(                                                                             \
	_Generic(s,                                                           \
		const char *:  a2si_c,                                        \
		const void *:  a2si_c,                                        \
		char *:        a2si_nc,                                       \
		void *:        a2si_nc                                        \
	)(n, s, __VA_ARGS__)                                                  \
)

#define a2sl(n, s, ...)                                                       \
(                                                                             \
	_Generic(s,                                                           \
		const char *:  a2sl_c,                                        \
		const void *:  a2sl_c,                                        \
		char *:        a2sl_nc,                                       \
		void *:        a2sl_nc                                        \
	)(n, s, __VA_ARGS__)                                                  \
)

#define a2sll(n, s, ...)                                                      \
(                                                                             \
	_Generic(s,                                                           \
		const char *:  a2sll_c,                                       \
		const void *:  a2sll_c,                                       \
		char *:        a2sll_nc,                                      \
		void *:        a2sll_nc                                       \
	)(n, s, __VA_ARGS__)                                                  \
)


#endif  // include guard
