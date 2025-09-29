// SPDX-FileCopyrightText: 2012, Eric Biederman
// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_SHADOW_SUBID_SGETSIENT_H_
#define SHADOW_INCLUDE_LIB_SHADOW_SUBID_SGETSIENT_H_


#include "config.h"

#include "../libsubid/subid.h"


#ifdef ENABLE_SUBIDS
struct subordinate_range *sgetsient(const char *s);
#endif


#endif
