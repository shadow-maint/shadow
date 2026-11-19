// SPDX-FileCopyrightText: 2011, Peter Vrabec <pvrabec@redhat.com>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include "lsm/selinux/check.h"

#ifdef WITH_SELINUX

#include <stdio.h>
#include "defines.h"

#include <selinux/selinux.h>
#include <selinux/label.h>

#include "io/fprintf.h"
#include "io/syslog.h"
#include "prototypes.h"
#include "shadowlog.h"
#include "string/sprintf/aprintf.h"


/*
 *  Log callback for libselinux internal error reporting.
 */
format_attr(printf, 2, 3)
static int selinux_log_cb (int type, const char *fmt, ...) {
	va_list ap;
	char *buf;
#ifdef WITH_AUDIT
	static int selinux_audit_fd = -2;
#endif

	va_start (ap, fmt);
	buf = vaprintf(fmt, ap);
	va_end (ap);
	if (buf == NULL)
		return 0;

#ifdef WITH_AUDIT
	if (-2 == selinux_audit_fd) {
		selinux_audit_fd = audit_open ();

		if (-1 == selinux_audit_fd) {
			/* You get these only when the kernel doesn't have
			 * audit compiled in. */
			if (   (errno != EINVAL)
			    && (errno != EPROTONOSUPPORT)
			    && (errno != EAFNOSUPPORT)) {

			    (void) fputs (_("Cannot open audit interface.\n"),
			              log_get_logfd());
			    SYSLOG(LOG_WARN, "Cannot open audit interface.");
			}
		}
	}

	if (-1 != selinux_audit_fd) {
		if (SELINUX_AVC == type) {
			if (audit_log_user_avc_message (selinux_audit_fd,
			    AUDIT_USER_AVC, buf, NULL, NULL,
			    NULL, 0) > 0) {
				goto skip_syslog;
			}
		} else if (SELINUX_ERROR == type) {
			if (audit_log_user_avc_message (selinux_audit_fd,
			    AUDIT_USER_SELINUX_ERR, buf, NULL, NULL,
			    NULL, 0) > 0) {
				goto skip_syslog;
			}
		}
	}
#endif

	SYSLOG(LOG_WARN, "libselinux: %s", buf);

skip_syslog:
	free (buf);

	return 0;
}

/*
 * check_selinux_permit - Check whether SELinux grants the given
 *                        operation
 *
 *   Parameter is the SELinux permission name, e.g. rootok
 *
 *   Returns 0 when permission is granted
 *                  or something failed but running in
 *                  permissive mode
 */
int check_selinux_permit (const char *perm_name)
{
	char *user_context_raw;
	int r;

	if (0 == is_selinux_enabled ()) {
		return 0;
	}

	selinux_set_callback (SELINUX_CB_LOG, (union selinux_callback) { .func_log = selinux_log_cb });

	if (getprevcon_raw (&user_context_raw) != 0) {
		fprinte(log_get_logfd(), _("%s: can not get previous SELinux process context"),
		        log_get_progname());
		SYSLOGE(LOG_WARN, "can not get previous SELinux process context");
		return (security_getenforce () != 0);
	}

	r = selinux_check_access (user_context_raw, user_context_raw, "passwd", perm_name, NULL);
	freecon (user_context_raw);
	return r;
}
#endif  /* WITH_SELINUX */
