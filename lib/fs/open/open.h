// SPDX-FileCopyrightText: 2026, Serge Hallyn <serge@hallyn.com>
// SPDX-FileCopyrightText: 2026, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_FS_OPEN_OPEN_H_
#define SHADOW_INCLUDE_LIB_FS_OPEN_OPEN_H_


#include "config.h"


extern int samedev_openat(int dirfd, const char *path, int flags);


#endif  // include guard
