// SPDX-FileCopyrightText: 2011, Peter Vrabec <pvrabec@redhat.com>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include "lsm/selinux/context.h"

#ifdef WITH_SELINUX

#include <stdio.h>
#include "defines.h"

#include <selinux/selinux.h>
#include <selinux/label.h>

#include "prototypes.h"


static bool selinux_checked = false;
static bool selinux_enabled;
static /*@null@*/struct selabel_handle *selabel_hnd = NULL;


static void cleanup(void)
{
	if (selabel_hnd) {
		selabel_close(selabel_hnd);
		selabel_hnd = NULL;
	}
}


void reset_selinux_handle (void)
{
    cleanup();
}

/*
 * set_selinux_file_context - Set the security context before any file or
 *                            directory creation.
 *
 *	set_selinux_file_context () should be called before any creation
 *	of file, symlink, directory, ...
 *
 *	Callers may have to Reset SELinux to create files with default
 *	contexts with reset_selinux_file_context
 */
int set_selinux_file_context (const char *dst_name, mode_t mode)
{
	if (!selinux_checked) {
		selinux_enabled = is_selinux_enabled () > 0;
		selinux_checked = true;
	}

	if (selinux_enabled) {
		/* Get the default security context for this file */

		/*@null@*/char *fcontext_raw = NULL;
		int r;

		if (selabel_hnd == NULL) {
			selabel_hnd = selabel_open(SELABEL_CTX_FILE, NULL, 0);
			if (selabel_hnd == NULL) {
				return security_getenforce () != 0;
			}
			(void) atexit(cleanup);
		}

		r = selabel_lookup_raw(selabel_hnd, &fcontext_raw, dst_name, mode);
		if (r < 0) {
			/* No context specified for the searched path */
			if (errno == ENOENT) {
				return 0;
			}

			return security_getenforce () != 0;
		}

		/* Set the security context for the next created file */
		r = setfscreatecon_raw (fcontext_raw);
		freecon (fcontext_raw);
		if (r < 0) {
			return security_getenforce () != 0;
		}
	}
	return 0;
}

/*
 * reset_selinux_file_context - Reset the security context to the default
 *                              policy behavior
 *
 *	reset_selinux_file_context () should be called after the context
 *	was changed with set_selinux_file_context ()
 */
int reset_selinux_file_context (void)
{
	if (!selinux_checked) {
		selinux_enabled = is_selinux_enabled () > 0;
		selinux_checked = true;
	}
	if (selinux_enabled) {
		if (setfscreatecon_raw (NULL) != 0) {
			return security_getenforce () != 0;
		}
	}
	return 0;
}
#endif  /* WITH_SELINUX */
