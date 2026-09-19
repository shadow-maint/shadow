// SPDX-FileCopyrightText: 2026, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_SHADOW_PASSWD_GETPW_H_
#define SHADOW_INCLUDE_LIB_SHADOW_PASSWD_GETPW_H_


#include "config.h"

#include <errno.h>
#include <pwd.h>
#include <stdbool.h>
#include <sys/types.h>

#include "atoi/getnum.h"


inline const struct passwd *getpw_uid_or_nam(const char *u);


// getpw_uid_or_nam - getpwuid(3) or getpwnam(3)
inline const struct passwd *
getpw_uid_or_nam(const char *u)
{
	int    e;
	bool   isuid;
	uid_t  uid;

	e = errno;
	isuid = get_uid(u, &uid) == 0;
	errno = e;

	return isuid ? getpwuid(uid) : getpwnam(u);
}


#endif  // include guard
