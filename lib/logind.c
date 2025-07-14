// SPDX-FileCopyrightText: 2023, Iker Pedrosa <ipedrosa@redhat.com>
// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include "session_management.h"

#include <errno.h>
#include <pwd.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include <systemd/sd-login.h>

#include "attr.h"
#include "defines.h"
#include "prototypes.h"


ATTR_MALLOC(free) static char *pid_get_session(pid_t pid);


int
get_session_host(char **out, pid_t main_pid)
{
	int   ret;
	char  *host;
	char  *session;

	session = pid_get_session(main_pid);
	if (session == NULL)
		return errno;

	ret = sd_session_get_remote_host(session, &host);
	free(session);
	if (ret < 0)
		return ret;

	*out = host;
	return 0;
}


unsigned long
active_sessions_count(const char *name, unsigned long)
{
	struct passwd  *pw;

	pw = prefix_getpwnam(name);
	if (pw == NULL)
		return 0;

	return sd_uid_get_sessions(pw->pw_uid, 0, NULL);
}


static char *
pid_get_session(pid_t pid)
{
	int   e;
	char  *session;

	e = sd_pid_get_session(pid, &session);
	if (e < 0) {
		errno = -e;
		return NULL;
	}

	return session;
}
