// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_SHADOW_PASSWD_SGETPWENT_H_
#define SHADOW_INCLUDE_LIB_SHADOW_PASSWD_SGETPWENT_H_


#include "config.h"

#include <pwd.h>


struct passwd *sgetpwent(const char *s);


#endif  // include guard
