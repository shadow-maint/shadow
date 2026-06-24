// SPDX-FileCopyrightText: 2019-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_TYPETRAITS_H_
#define SHADOW_INCLUDE_LIB_TYPETRAITS_H_


#include "config.h"

#include "sizeof.h"


#if (__GNUC__ >= 16)
# define maxof(T)  _Maxof(T)
# define minof(T)  _Minof(T)
#else
# define is_unsigned(x)  ((typeof(x)) -1 > 1)
# define is_signed(x)    ((typeof(x)) -1 < 1)
# define stype_max(T)    ((T) (((((T) 1 << (WIDTHOF(T) - 2)) - 1) << 1) + 1))
# define utype_max(T)    ((T) -1)
# define maxof(T)        ((T) (is_signed(T) ? stype_max(T) : utype_max(T)))
# define minof(T)        ((T) ~maxof(T))
#endif


#define is_same_type(a, b)                                                    \
(                                                                             \
	__builtin_types_compatible_p(a, b)                                    \
)

#define is_same_typeof(a, b)                                                  \
(                                                                             \
	is_same_type(typeof(a), typeof(b))                                    \
)


#define QChar_of(s)  typeof                                           \
(                                                                     \
	_Generic(s,                                                   \
		const char *:  (const char){0},                       \
		const void *:  (const char){0},                       \
		char *:        (char){0},                             \
		void *:        (char){0}                              \
	)                                                             \
)


#endif  // include guard
