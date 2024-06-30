// SPDX-FileCopyrightText: 2023-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ATOI_A2I_A2I_H_
#define SHADOW_INCLUDE_LIB_ATOI_A2I_A2I_H_


#include <config.h>

#include "atoi/a2i/a2s_c.h"
#include "atoi/a2i/a2s_nc.h"
#include "atoi/a2i/a2u_c.h"
#include "atoi/a2i/a2u_nc.h"


/*
 * See the manual of these macros in liba2i's documentation:
 * <http://www.alejandro-colomar.es/share/dist/liba2i/git/HEAD/liba2i-HEAD.pdf>
 */


#define a2i(TYPE, n, s, ...)                                                  \
(                                                                             \
	_Generic((void (*)(TYPE, typeof(s))) 0,                               \
		void (*)(short,              const char *):  a2sh_c,          \
		void (*)(short,              const void *):  a2sh_c,          \
		void (*)(short,              char *):        a2sh_nc,         \
		void (*)(short,              void *):        a2sh_nc,         \
		void (*)(int,                const char *):  a2si_c,          \
		void (*)(int,                const void *):  a2si_c,          \
		void (*)(int,                char *):        a2si_nc,         \
		void (*)(int,                void *):        a2si_nc,         \
		void (*)(long,               const char *):  a2sl_c,          \
		void (*)(long,               const void *):  a2sl_c,          \
		void (*)(long,               char *):        a2sl_nc,         \
		void (*)(long,               void *):        a2sl_nc,         \
		void (*)(long long,          const char *):  a2sll_c,         \
		void (*)(long long,          const void *):  a2sll_c,         \
		void (*)(long long,          char *):        a2sll_nc,        \
		void (*)(long long,          void *):        a2sll_nc,        \
		void (*)(unsigned short,     const char *):  a2uh_c,          \
		void (*)(unsigned short,     const void *):  a2uh_c,          \
		void (*)(unsigned short,     char *):        a2uh_nc,         \
		void (*)(unsigned short,     void *):        a2uh_nc,         \
		void (*)(unsigned int,       const char *):  a2ui_c,          \
		void (*)(unsigned int,       const void *):  a2ui_c,          \
		void (*)(unsigned int,       char *):        a2ui_nc,         \
		void (*)(unsigned int,       void *):        a2ui_nc,         \
		void (*)(unsigned long,      const char *):  a2ul_c,          \
		void (*)(unsigned long,      const void *):  a2ul_c,          \
		void (*)(unsigned long,      char *):        a2ul_nc,         \
		void (*)(unsigned long,      void *):        a2ul_nc,         \
		void (*)(unsigned long long, const char *):  a2ull_c,         \
		void (*)(unsigned long long, const void *):  a2ull_c,         \
		void (*)(unsigned long long, char *):        a2ull_nc,        \
		void (*)(unsigned long long, void *):        a2ull_nc         \
	)(n, s, __VA_ARGS__)                                                  \
)


#endif  // include guard
