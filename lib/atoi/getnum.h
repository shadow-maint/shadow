// SPDX-FileCopyrightText: 2009, Nicolas François
// SPDX-FileCopyrightText: 2023, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ATOI_GETNUM_H_
#define SHADOW_INCLUDE_LIB_ATOI_GETNUM_H_


#include <config.h>

#include <stddef.h>
#include <sys/types.h>

#include "atoi/getlong.h"
#include "attr.h"
#include "typetraits.h"


#define getnum(TYPE, ...)                                                     \
(                                                                             \
	_Generic((TYPE) 0,                                                    \
		long:               getlong,                                  \
		long long:          getllong,                                 \
		unsigned long:      getulong,                                 \
		unsigned long long: getullong                                 \
	)(__VA_ARGS__)                                                        \
)


#define getn(TYPE, ...)                                                       \
(                                                                             \
	_Generic((TYPE) 0,                                                    \
		long:               getl,                                     \
		long long:          getll,                                    \
		unsigned long:      getul,                                    \
		unsigned long long: getull                                    \
	)(__VA_ARGS__)                                                        \
)


ATTR_STRING(1) ATTR_ACCESS(write_only, 2)
inline int get_gid(const char *restrict gidstr, gid_t *restrict gid);


inline int
get_gid(const char *restrict gidstr, gid_t *restrict gid)
{
	return getnum(gid_t, gidstr, gid,
	              NULL, 10, type_min(gid_t), type_max(gid_t));
}


#endif  // include guard
