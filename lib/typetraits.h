// SPDX-FileCopyrightText: 2019-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_TYPETRAITS_H_
#define SHADOW_INCLUDE_LIB_TYPETRAITS_H_


#include "config.h"

#include "sizeof.h"


#define is_unsigned(x)                                                        \
(                                                                             \
	(typeof(x)) -1 > 1                                                    \
)

#define is_signed(x)                                                          \
(                                                                             \
	(typeof(x)) -1 < 1                                                    \
)


#define stype_max(T)                                                          \
(                                                                             \
	(T) (((((T) 1 << (WIDTHOF(T) - 2)) - 1) << 1) + 1)                    \
)

#define utype_max(T)                                                          \
(                                                                             \
	(T) -1                                                                \
)

#define type_max(T)                                                           \
(                                                                             \
	(T) (is_signed(T) ? stype_max(T) : utype_max(T))                      \
)

#define type_min(T)                                                           \
(                                                                             \
	(T) ~type_max(T)                                                      \
)


#define is_same_type(a, b)                                                    \
(                                                                             \
	__builtin_types_compatible_p(a, b)                                    \
)

#define is_same_typeof(a, b)                                                  \
(                                                                             \
	is_same_type(typeof(a), typeof(b))                                    \
)


#define QVoidptrof_(p)  typeof(1?(p):(void*){0})
#define QVoid_of(p)     typeof((QVoidptrof_(p)){0}[0])
#define QChar_of(Q, s)  typeof                                        \
(                                                                     \
	_Generic((QVoid_of(s) *){0},                                  \
		Q void *:  (Q char){0},                               \
		void *:    (char){0}                                  \
	)                                                             \
)
#define CQChar_of(s)    QChar_of(const, s)
#define VQChar_of(s)    QChar_of(volatile, s)


#endif  // include guard
