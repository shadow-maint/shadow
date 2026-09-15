/* SPDX-License-Identifier: BSD-3-Clause */

/* A passwd NSS module whose backend is always unavailable. */

#include <errno.h>
#include <nss.h>
#include <pwd.h>
#include <stddef.h>
#include <sys/types.h>

#include "attr.h"

enum nss_status
_nss_fail_getpwnam_r(MAYBE_UNUSED const char *name,
    MAYBE_UNUSED struct passwd *pwd, MAYBE_UNUSED char *buf,
    MAYBE_UNUSED size_t buflen, int *errnop)
{
	*errnop = EAGAIN;
	return NSS_STATUS_UNAVAIL;
}

enum nss_status
_nss_fail_getpwuid_r(MAYBE_UNUSED uid_t uid, MAYBE_UNUSED struct passwd *pwd,
    MAYBE_UNUSED char *buf, MAYBE_UNUSED size_t buflen, int *errnop)
{
	*errnop = EAGAIN;
	return NSS_STATUS_UNAVAIL;
}
