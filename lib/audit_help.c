/*
 * SPDX-FileCopyrightText: 2005       , Red Hat, Inc.
 * SPDX-FileCopyrightText: 2005       , Tomasz Kłoczko
 * SPDX-FileCopyrightText: 2008       , Nicolas François
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 *  Audit helper functions used throughout shadow
 *
 */

#include <config.h>

#ifdef WITH_AUDIT

#include <stdlib.h>
#include <syslog.h>
#include <stdarg.h>
#include <libaudit.h>
#include <errno.h>
#include <stdio.h>

#include "attr.h"
#include "prototypes.h"
#include "shadowlog.h"
#include "string/sprintf/snprintf.h"

int audit_fd;

void audit_help_open (void)
{
	audit_fd = audit_open ();
	if (audit_fd < 0) {
		/* You get these only when the kernel doesn't have
		 * audit compiled in. */
		if (   (errno == EINVAL)
		    || (errno == EPROTONOSUPPORT)
		    || (errno == EAFNOSUPPORT)) {
			return;
		}
		(void) fputs (_("Cannot open audit interface - aborting.\n"),
		              log_get_logfd());
		exit (EXIT_FAILURE);
	}
}

/*
 * This function will log a message to the audit system using a predefined
 * message format. For additional information on the user account lifecycle
 * events check
 * <https://github.com/linux-audit/audit-documentation/wiki/SPEC-User-Account-Lifecycle-Events>
 *
 * Parameter usage is as follows:
 *
 * type - type of message. A list of possible values is available in
 *        "audit-records.h" file.
 * pgname - program's name
 * op  -  operation. "adding user", "changing finger info", "deleting group"
 * name - user's account or group name. If not available use NULL.
 * id  -  uid or gid that the operation is being performed on. This is used
 *	  only when user is NULL.
 */
void audit_logger (int type, MAYBE_UNUSED const char *pgname, const char *op,
                   const char *name, unsigned int id,
                   shadow_audit_result result)
{
	if (audit_fd < 0) {
		return;
	} else {
		audit_log_acct_message (audit_fd, type, NULL, op, name, id,
		                        NULL, NULL, NULL, result);
	}
}

/*
 * This function will log a message to the audit system using a predefined
 * message format. For additional information on the group account lifecycle
 * events check
 * <https://github.com/linux-audit/audit-documentation/wiki/SPEC-User-Account-Lifecycle-Events>
 *
 * Parameter usage is as follows:
 *
 * type - type of message. A list of possible values is available in
 *        "audit-records.h" file.
 * op  -  operation. "adding-user", "modify-group", "deleting-user-from-group"
 * name - user's account or group name. If not available use NULL.
 * id  -  uid or gid that the operation is being performed on. This is used
 *	  only when user is NULL.
 * grp_type - type of group: "grp" or "new_group"
 * grp - group name associated with event
 */
void
audit_logger_with_group(int type, const char *op, const char *name,
    id_t id, const char *grp_type, const char *grp,
    shadow_audit_result result)
{
	int len;
	char enc_group[GROUP_NAME_MAX_LENGTH * 2 + 1];
	char buf[countof(enc_group) + 100];

	if (audit_fd < 0)
		return;

	len = strnlen(grp, sizeof(enc_group)/2);
	if (audit_value_needs_encoding(grp, len)) {
		SNPRINTF(buf, "%s %s=%s", op, grp_type,
			audit_encode_value(enc_group, grp, len));
	} else {
		SNPRINTF(buf, "%s %s=\"%s\"", op, grp_type, grp);
	}

	audit_log_acct_message(audit_fd, type, NULL, buf, name, id,
		               NULL, NULL, NULL, result);
}

void audit_logger_message (const char *message, shadow_audit_result result)
{
	if (audit_fd < 0) {
		return;
	} else {
		audit_log_user_message (audit_fd,
		                        AUDIT_USYS_CONFIG,
		                        message,
		                        NULL, /* hostname */
		                        NULL, /* addr */
		                        NULL, /* tty */
		                        result);
	}
}

#else				/* WITH_AUDIT */
extern int ISO_C_forbids_an_empty_translation_unit;
#endif				/* WITH_AUDIT */

