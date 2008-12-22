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
#include "groupio.h"
#include "sgroupio.h"
#include "prototypes.h"

/*
 * cleanup_report_add_group - Report failure to add a group to the system
 *
 * It should be registered when it is decided to add a group to the system.
 */
void cleanup_report_add_group (void *group_name)
{
	const char *name = (const char *)group_name;

	SYSLOG ((LOG_ERR, "failed to add group %s", name));
#ifdef WITH_AUDIT
	audit_logger (AUDIT_ADD_GROUP, Prog,
	              "",
	              name, AUDIT_NO_ID,
	              SHADOW_AUDIT_FAILURE);
#endif
}

/*
 * cleanup_report_del_group - Report failure to remove a group from the system
 *
 * It should be registered when it is decided to remove a group from the system.
 */
void cleanup_report_del_group (void *group_name)
{
	const char *name = (const char *)group_name;

	SYSLOG ((LOG_ERR, "failed to remove group %s", name));
#ifdef WITH_AUDIT
	audit_logger (AUDIT_DEL_GROUP, Prog,
	              "",
	              name, AUDIT_NO_ID,
	              SHADOW_AUDIT_FAILURE);
#endif
}

void cleanup_report_mod_group (void *cleanup_info)
{
	const struct cleanup_info_mod *info;
	info = (const struct cleanup_info_mod *)cleanup_info;

	SYSLOG ((LOG_ERR,
	         "failed to change %s (%s)",
	         gr_dbname (),
	         info->action));
#ifdef WITH_AUDIT
	audit_logger (AUDIT_USER_ACCT, Prog,
	              info->audit_msg,
	              info->name, AUDIT_NO_ID,
	              SHADOW_AUDIT_FAILURE);
#endif
}

void cleanup_report_mod_gshadow (void *cleanup_info)
{
	const struct cleanup_info_mod *info;
	info = (const struct cleanup_info_mod *)cleanup_info;

	SYSLOG ((LOG_ERR,
	         "failed to change %s (%s)",
	         sgr_dbname (),
	         info->action));
#ifdef WITH_AUDIT
	audit_logger (AUDIT_USER_ACCT, Prog,
	              info->audit_msg,
	              info->name, AUDIT_NO_ID,
	              SHADOW_AUDIT_FAILURE);
#endif
}

/*
 * cleanup_report_add_group_group - Report failure to add a group to group
 *
 * It should be registered when it is decided to add a group to the
 * group database.
 */
void cleanup_report_add_group_group (void *group_name)
{
	const char *name = (const char *)group_name;

	SYSLOG ((LOG_ERR, "failed to add group %s to %s", name, gr_dbname ()));
#ifdef WITH_AUDIT
	audit_logger (AUDIT_ADD_GROUP, Prog,
	              "adding group to /etc/group",
	              name, AUDIT_NO_ID,
	              SHADOW_AUDIT_FAILURE);
#endif
}

#ifdef SHADOWGRP
/*
 * cleanup_report_add_group_gshadow - Report failure to add a group to gshadow
 *
 * It should be registered when it is decided to add a group to the
 * gshadow database.
 */
void cleanup_report_add_group_gshadow (void *group_name)
{
	const char *name = (const char *)group_name;

	SYSLOG ((LOG_ERR, "failed to add group %s to %s", name, sgr_dbname ()));
#ifdef WITH_AUDIT
	audit_logger (AUDIT_ADD_GROUP, Prog,
	              "adding group to /etc/gshadow",
	              name, AUDIT_NO_ID,
	              SHADOW_AUDIT_FAILURE);
#endif
}
#endif

/*
 * cleanup_report_del_group_group - Report failure to remove a group from the
 *                                  regular group database
 *
 * It should be registered when it is decided to remove a group from the
 * regular group database.
 */
void cleanup_report_del_group_group (void *group_name)
{
	const char *name = (const char *)group_name;

	SYSLOG ((LOG_ERR,
	         "failed to remove group %s from %s",
	         name, gr_dbname ()));
#ifdef WITH_AUDIT
	audit_logger (AUDIT_ADD_GROUP, Prog,
	              "removing group from /etc/group",
	              name, AUDIT_NO_ID,
	              SHADOW_AUDIT_FAILURE);
#endif
}

#ifdef SHADOWGRP
/*
 * cleanup_report_del_group_gshadow - Report failure to remove a group from
 *                                    gshadow
 *
 * It should be registered when it is decided to remove a group from the
 * gshadow database.
 */
void cleanup_report_del_group_gshadow (void *group_name)
{
	const char *name = (const char *)group_name;

	SYSLOG ((LOG_ERR,
	         "failed to remove group %s from %s",
	         name, sgr_dbname ()));
#ifdef WITH_AUDIT
	audit_logger (AUDIT_ADD_GROUP, Prog,
	              "removing group from /etc/gshadow",
	              name, AUDIT_NO_ID,
	              SHADOW_AUDIT_FAILURE);
#endif
}
#endif

/*
 * cleanup_unlock_group - Unlock the group file
 *
 * It should be registered after the group file is successfully locked.
 */
void cleanup_unlock_group (unused void *arg)
{
	if (gr_unlock () == 0) {
		fprintf (stderr,
		         _("%s: failed to unlock %s\n"),
		         Prog, gr_dbname ());
		SYSLOG ((LOG_ERR, "failed to unlock %s", gr_dbname ()));
#ifdef WITH_AUDIT
		audit_logger_message ("unlocking group file",
		                      SHADOW_AUDIT_FAILURE);
#endif
	}
}

#ifdef SHADOWGRP
/*
 * cleanup_unlock_gshadow - Unlock the gshadow file
 *
 * It should be registered after the gshadow file is successfully locked.
 */
void cleanup_unlock_gshadow (unused void *arg)
{
	if (sgr_unlock () == 0) {
		fprintf (stderr,
		         _("%s: failed to unlock %s\n"),
		         Prog, sgr_dbname ());
		SYSLOG ((LOG_ERR, "failed to unlock %s", sgr_dbname ()));
#ifdef WITH_AUDIT
		audit_logger_message ("unlocking gshadow file",
		                      SHADOW_AUDIT_FAILURE);
#endif
	}
}
#endif

