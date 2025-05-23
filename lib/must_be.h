/*
 * SPDX-FileCopyrightText: 2019-2023, Alejandro Colomar <alx@kernel.org>
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef SHADOW_INCLUDE_LIBMISC_MUST_BE_H_
#define SHADOW_INCLUDE_LIBMISC_MUST_BE_H_


#include <config.h>


#define is_same_type(a, b)                                                    \
(                                                                             \
	__builtin_types_compatible_p(a, b)                                    \
)


#define is_same_typeof(a, b)                                                  \
(                                                                             \
	is_same_type(typeof(a), typeof(b))                                    \
)


#endif  // include guard
