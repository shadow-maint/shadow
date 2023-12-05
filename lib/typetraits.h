/*
 * SPDX-FileCopyrightText: 2022-2023, Alejandro Colomar <alx@kernel.org>
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef SHADOW_INCLUDE_LIB_TYPETRAITS_H_
#define SHADOW_INCLUDE_LIB_TYPETRAITS_H_


#include <config.h>

#include <stddef.h>


#define is_same_type(a, b)                                                    \
(                                                                             \
	__builtin_types_compatible_p(typeof(a), typeof(b))                    \
)


#define is_pointer_or_array(p)                                                \
(                                                                             \
	__builtin_classify_type(p) == 5                                       \
)


#define decay_array(p)                                                        \
(                                                                             \
	&*__builtin_choose_expr(is_pointer_or_array(p), p, NULL)              \
)


#if (__GNUC__ >= 14)
# define is_pointer(p)                                                        \
(                                                                             \
	__builtin_classify_type(typeof(p)) == 5                               \
)
#else
# define is_pointer(p)                                                        \
(                                                                             \
	is_same_type(p, decay_array(p))                                       \
)
#endif


#if (__GNUC__ >= 14)
# define is_array(a)                                                          \
(                                                                             \
	__builtin_classify_type(typeof(a)) == 14                              \
)
#else
# define is_array(a)                                                          \
(                                                                             \
	is_pointer_or_array(a) && !is_pointer(a)                              \
)
#endif


#endif  // include guard
