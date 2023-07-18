/*
 * SPDX-FileCopyrightText:  2023, Iker Pedrosa <ipedrosa@redhat.com>
 *
 * SPDX-License-Identifier:  BSD-3-Clause
 */

#include <config.h>

#ident "$Id$"

#include "defines.h"
#include "prototypes.h"

#include <systemd/sd-login.h>

int get_session_host (char **out)
{
    char *host = NULL;
    char *session = NULL;
    int ret;

    ret = sd_pid_get_session (getpid(), &session);
    if (ret < 0) {
        return ret;
    }
    ret = sd_session_get_remote_host (session, &host);
    if (ret < 0) {
        goto done;
    }

    *out = host;

done:
    free (session);
    return ret;
}
