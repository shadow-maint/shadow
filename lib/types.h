/*
 * SPDX-FileCopyrightText: 2022-2023, Alejandro Colomar <alx@kernel.org>
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef SHADOW_INCLUDE_LIB_TYPES_H_
#define SHADOW_INCLUDE_LIB_TYPES_H_


#include <config.h>

#include "sizeof.h"


#define is_unsigned(x)  (((typeof(x)) -1) > 1)
#define is_signed(x)    (((typeof(x)) -1) < 1)

#define stype_max(t)    ((t) (((((t) 1 << (WIDTHOF(t) - 2)) - 1) << 1) + 1))
#define utype_max(t)    ((t) -1)
#define type_max(t)     ((t) (is_signed(t) ? stype_max(t) : utype_max(t)))
#define type_min(t)     ((t) ~type_max(t))


#endif  // include guard
