// SPDX-FileCopyrightText: 2026, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_LSM_SELINUX_CHECK_H_
#define SHADOW_INCLUDE_LIB_LSM_SELINUX_CHECK_H_


#include "config.h"


#ifdef WITH_SELINUX
extern int check_selinux_permit (const char *perm_name);
#endif


#endif  // include guard
