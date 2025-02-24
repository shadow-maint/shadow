// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_SHADOW_SHADOW_SGETSPENT_H_
#define SHADOW_INCLUDE_LIB_SHADOW_SHADOW_SGETSPENT_H_


#include "config.h"

#include <shadow.h>


#if !defined(HAVE_SGETSPENT)
struct spwd *sgetspent(const char *s);
#endif


#endif  // include guard
