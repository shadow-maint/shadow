// SPDX-FileCopyrightText: 2009, Nicolas François
// SPDX-FileCopyrightText: 2023-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include <config.h>

#include <sys/types.h>

#include "atoi/getnum.h"


extern inline int get_gid(const char *restrict gidstr, gid_t *restrict gid);
extern inline int get_pid(const char *restrict pidstr, pid_t *restrict pid);
