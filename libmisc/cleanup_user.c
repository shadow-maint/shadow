/*
 * Copyright (c) 2008       , Nicolas François
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the copyright holders or contributors may not be used to
 *    endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <config.h>

#include <assert.h>
#include <stdio.h>

#include "defines.h"
#include "pwio.h"
#include "shadowio.h"
#include "prototypes.h"

/*
 * cleanup_report_add_user - Report failure to add an user to the system
 *
 * It should be registered when it is decided to add an user to the system.
 */
void cleanup_report_add_user (void *user_name)
{
	const char *name = (const char *)user_name;

	SYSLOG ((LOG_ERR, "failed to add user %s", name));
#ifdef WITH_AUDIT
	audit_logger (AUDIT_ADD_USER, Prog,
	              "",
	              name, AUDIT_NO_ID,
	              SHADOW_AUDIT_FAILURE);
#endif
}

void cleanup_report_mod_passwd (void *cleanup_info)
{
	const struct cleanup_info_mod *info;
	info = (const struct cleanup_info_mod *)cleanup_info;

	SYSLOG ((LOG_ERR,
	         "failed to change %s (%s)",
	         pw_dbname (),
	         info->action));
#ifdef WITH_AUDIT
	audit_logger (AUDIT_USER_ACCT, Prog,
	              info->audit_msg,
	              info->name, AUDIT_NO_ID,
	              SHADOW_AUDIT_FAILURE);
#endif
}

/*
 * cleanup_report_add_user_passwd - Report failure to add an user to
 * /etc/passwd
 *
 * It should be registered when it is decided to add an user to the
 * /etc/passwd database.
 */
void cleanup_report_add_user_passwd (void *user_name)
{
	const char *name = (const char *)user_name;

	SYSLOG ((LOG_ERR, "failed to add user %s to %s", name, pw_dbname ()));
#ifdef WITH_AUDIT
	audit_logger (AUDIT_ADD_USER, Prog,
	              "adding user to /etc/passwd",
	              name, AUDIT_NO_ID,
	              SHADOW_AUDIT_FAILURE);
#endif
}

/*
 * cleanup_report_add_user_shadow - Report failure to add an user to
 * /etc/shadow
 *
 * It should be registered when it is decided to add an user to the
 * /etc/shadow database.
 */
void cleanup_report_add_user_shadow (void *user_name)
{
	const char *name = (const char *)user_name;

	SYSLOG ((LOG_ERR, "failed to add user %s to %s", name, spw_dbname ()));
#ifdef WITH_AUDIT
	audit_logger (AUDIT_ADD_USER, Prog,
	              "adding user to /etc/shadow",
	              name, AUDIT_NO_ID,
	              SHADOW_AUDIT_FAILURE);
#endif
}

/*
 * cleanup_unlock_passwd - Unlock the /etc/passwd database
 *
 * It should be registered after the passwd database is successfully locked.
 */
void cleanup_unlock_passwd (unused void *arg)
{
	if (pw_unlock () == 0) {
		fprintf (shadow_logfd,
		         _("%s: failed to unlock %s\n"),
		         Prog, pw_dbname ());
		SYSLOG ((LOG_ERR, "failed to unlock %s", pw_dbname ()));
#ifdef WITH_AUDIT
		audit_logger_message ("unlocking passwd file",
		                      SHADOW_AUDIT_FAILURE);
#endif
	}
}

/*
 * cleanup_unlock_shadow - Unlock the /etc/shadow database
 *
 * It should be registered after the shadow database is successfully locked.
 */
void cleanup_unlock_shadow (unused void *arg)
{
	if (spw_unlock () == 0) {
		fprintf (shadow_logfd,
		         _("%s: failed to unlock %s\n"),
		         Prog, spw_dbname ());
		SYSLOG ((LOG_ERR, "failed to unlock %s", spw_dbname ()));
#ifdef WITH_AUDIT
		audit_logger_message ("unlocking shadow file",
		                      SHADOW_AUDIT_FAILURE);
#endif
	}
}

