// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_SHADOW_GROUP_SGETGRENT_H_
#define SHADOW_INCLUDE_LIB_SHADOW_GROUP_SGETGRENT_H_


#include "config.h"

#include <grp.h>


struct group *sgetgrent(const char *s);


#endif  // include guard
