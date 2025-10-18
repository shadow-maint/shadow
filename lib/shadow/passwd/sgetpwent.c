// SPDX-FileCopyrightText: 1989-1994, Julianne Frances Haugh
// SPDX-FileCopyrightText: 1996-1998, Marek Michałkiewicz
// SPDX-FileCopyrightText: 2003-2005, Tomasz Kłoczko
// SPDX-FileCopyrightText: 2008, Nicolas François
// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
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


// from-string get pasword entry
struct passwd *
sgetpwent(const char *s)
{
	static char           *buf = NULL;
	static struct passwd  pwent_ = {};
	struct passswd        *pwent = &pwent_;

	char    *fields[7];
	size_t  size;

	size = strlen(s) + 1;

	free(buf);
	buf = MALLOC(size, char);
	if (buf == NULL)
		return NULL;

	if (strtcpy(buf, s, size) == -1)
		return NULL;

	stpsep(buf, "\n");

	if (STRSEP2ARR(buf, ":", fields) == -1)
		return NULL;

	pwent->pw_name = fields[0];
	pwent->pw_passwd = fields[1];
	if (get_uid(fields[2], &pwent->pw_uid) == -1)
		return NULL;
	if (get_gid(fields[3], &pwent->pw_gid) == -1)
		return NULL;
	pwent->pw_gecos = fields[4];
	pwent->pw_dir = fields[5];
	pwent->pw_shell = fields[6];

	return pwent;
}
