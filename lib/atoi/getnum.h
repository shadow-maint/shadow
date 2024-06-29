// SPDX-FileCopyrightText: 2009, Nicolas François
// SPDX-FileCopyrightText: 2023-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ATOI_GETNUM_H_
#define SHADOW_INCLUDE_LIB_ATOI_GETNUM_H_


#include <config.h>

#include <stddef.h>
#include <sys/types.h>

#include "atoi/a2i.h"
#include "attr.h"
#include "typetraits.h"


ATTR_STRING(1) ATTR_ACCESS(write_only, 2)
inline int get_gid(const char *restrict gidstr, gid_t *restrict gid);
ATTR_STRING(1) ATTR_ACCESS(write_only, 2)
inline int get_pid(const char *restrict pidstr, pid_t *restrict pid);


inline int
get_gid(const char *restrict gidstr, gid_t *restrict gid)
{
	return a2i(gid_t, gid, gidstr, NULL, 10, type_min(gid_t), type_max(gid_t));
}


inline int
get_pid(const char *restrict pidstr, pid_t *restrict pid)
{
	return a2i(pid_t, pid, pidstr, NULL, 10, 1, type_max(pid_t));
}


#endif  // include guard
