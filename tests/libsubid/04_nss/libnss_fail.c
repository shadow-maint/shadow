/* SPDX-License-Identifier: BSD-3-Clause */

/*
 * A passwd NSS module whose backend is always unavailable, to check what
 * libsubid reports when the owner lookup cannot reach the passwd database.
 */

#include <errno.h>
#include <nss.h>
#include <pwd.h>
#include <stddef.h>

enum nss_status
_nss_fail_getpwnam_r(const char *name, struct passwd *pwd, char *buf,
                     size_t buflen, int *errnop)
{
	(void) name; (void) pwd; (void) buf; (void) buflen;
	*errnop = EAGAIN;
	return NSS_STATUS_UNAVAIL;
}

enum nss_status
_nss_fail_getpwuid_r(uid_t uid, struct passwd *pwd, char *buf,
                     size_t buflen, int *errnop)
{
	(void) uid; (void) pwd; (void) buf; (void) buflen;
	*errnop = EAGAIN;
	return NSS_STATUS_UNAVAIL;
}
