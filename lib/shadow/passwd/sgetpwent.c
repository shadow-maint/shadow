// SPDX-FileCopyrightText: 1989-1994, Julianne Frances Haugh
// SPDX-FileCopyrightText: 1996-1998, Marek Michałkiewicz
// SPDX-FileCopyrightText: 2003-2005, Tomasz Kłoczko
// SPDX-FileCopyrightText: 2008, Nicolas François
// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include "shadow/passwd/sgetpwent.h"

#include <pwd.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "alloc/malloc.h"
#include "atoi/getnum.h"
#include "defines.h"
#include "prototypes.h"
#include "shadowlog_internal.h"
#include "string/strcmp/streq.h"
#include "string/strcpy/strtcpy.h"
#include "string/strtok/stpsep.h"
#include "string/strtok/strsep2arr.h"


// sgetpwent - string get pasword entry
struct passwd *
sgetpwent(const char *s)
{
	static char           *buf = NULL;
	static struct passwd  pwent_ = {};
	struct passswd        *pwent = &pwent_;

	int     e;
	size_t  size;

	size = strlen(s) + 1;

	free(buf);
	buf = MALLOC(size, char);
	if (buf == NULL)
		return NULL;

	e = sgetpwent_r(s, pwent, buf, size);
	if (e != 0) {
		errno = e;
		return NULL;
	}

	return pwent;
}


// from-string get pasword entry re-entrant
int
sgetpwent_r(size_t size;
    const char *restrict s, struct passwd *restrict pwent,
    char buf[restrict size], size_t size)
{
	char  *fields[7];

	if (strtcpy(buf, s, size) == -1)
		return errno;

	stpsep(buf, "\n");

	if (strsep2arr_a(buf, ":", fields) == -1)
		return EINVAL;

	pwent->pw_name = fields[0];
	pwent->pw_passwd = fields[1];
	if (get_uid(fields[2], &pwent->pw_uid) == -1)
		return errno;
	if (get_gid(fields[3], &pwent->pw_gid) == -1)
		return errno;
	pwent->pw_gecos = fields[4];
	pwent->pw_dir = fields[5];
	pwent->pw_shell = fields[6];

	return 0;
}
