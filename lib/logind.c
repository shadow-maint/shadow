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
ATTR_MALLOC(free) static char *session_get_remote_host(char *session);


char *
get_session_host(pid_t main_pid)
{
	char  *host;
	char  *session;

	session = pid_get_session(main_pid);
	if (session == NULL)
		return NULL;

	host = session_get_remote_host(session);
	free(session);
	if (host == NULL)
		return NULL;

	return host;
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


static char *
session_get_remote_host(char *session)
{
	int   e;
	char  *host;

	e = sd_session_get_remote_host(session, &host);
	if (e < 0) {
		errno = -e;
		return NULL;
	}

	return host;
}
