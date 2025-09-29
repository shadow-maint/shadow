// SPDX-FileCopyrightText: 1989-1994, Julianne Frances Haugh
// SPDX-FileCopyrightText: 1996-1998, Marek Michałkiewicz
// SPDX-FileCopyrightText: 2003-2005, Tomasz Kłoczko
// SPDX-FileCopyrightText: 2008, Nicolas François
// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include "shadow/passwd/sgetpwent.h"

#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "atoi/getnum.h"
#include "defines.h"
#include "prototypes.h"
#include "shadowlog_internal.h"
#include "string/strcmp/streq.h"
#include "string/strtok/stpsep.h"
#include "string/strtok/strsep2arr.h"


// sgetpwent - string get pasword entry
struct passwd *
sgetpwent(const char *s)
{
	static char          *dup = NULL;
	static struct passwd pwent;

	char  *fields[7];

	free(dup);
	dup = strdup(s);
	if (dup == NULL)
		return NULL;

	stpsep(dup, "\n");

	if (strsep2arr_a(dup, ":", fields) == -1)
		return NULL;

	pwent.pw_name = fields[0];
	pwent.pw_passwd = fields[1];
	if (get_uid(fields[2], &pwent.pw_uid) == -1) {
		return NULL;
	}
	if (get_gid(fields[3], &pwent.pw_gid) == -1) {
		return NULL;
	}
	pwent.pw_gecos = fields[4];
	pwent.pw_dir = fields[5];
	pwent.pw_shell = fields[6];

	return &pwent;
}
