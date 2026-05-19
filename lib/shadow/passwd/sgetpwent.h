// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_SHADOW_PASSWD_SGETPWENT_H_
#define SHADOW_INCLUDE_LIB_SHADOW_PASSWD_SGETPWENT_H_


#include "config.h"

#include <pwd.h>
#include <stddef.h>


struct passwd *sgetpwent(const char *s);
int sgetpwent_r(size_t size;
    const char *restrict s, struct passwd *restrict pwent,
    char buf[restrict size], size_t size);


#endif  // include guard
