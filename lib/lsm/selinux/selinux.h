// SPDX-FileCopyrightText: 2026, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_LSM_SELINUX_SELINUX_H_
#define SHADOW_INCLUDE_LIB_LSM_SELINUX_SELINUX_H_


#include "config.h"

#include <sys/types.h>


#ifdef WITH_SELINUX
extern int set_selinux_file_context (const char *dst_name, mode_t mode);
extern void reset_selinux_handle (void);
extern int reset_selinux_file_context (void);
extern int check_selinux_permit (const char *perm_name);
#endif


#endif  // include guard
