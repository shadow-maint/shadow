// SPDX-FileCopyrightText: 2023, Iker Pedrosa <ipedrosa@redhat.com>
// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include "session_management.h"

#include <systemd/sd-login.h>

#include "attr.h"
#include "defines.h"
#include "prototypes.h"


int
get_session_host(char **out, pid_t main_pid)
{
	int   ret;
	char  *host;
	char  *session;

	ret = sd_pid_get_session(main_pid, &session);
	if (ret < 0)
		return ret;
	ret = sd_session_get_remote_host(session, &host);
	if (ret < 0)
		goto done;

	*out = host;

done:
	free(session);
	return ret;
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
