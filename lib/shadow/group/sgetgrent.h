// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_SHADOW_GROUP_SGETGRENT_H_
#define SHADOW_INCLUDE_LIB_SHADOW_GROUP_SGETGRENT_H_


#include "config.h"

#include <grp.h>
#include <stddef.h>


struct group *sgetgrent(const char *s);
int sgetgrent_r(size_t size;
    const char *restrict s, struct group *restrict grent,
    char buf[restrict size], size_t size);


#endif  // include guard
