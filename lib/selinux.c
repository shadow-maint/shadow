/*
 * Copyright (c) 2011       , Peter Vrabec <pvrabec@redhat.com>
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

#ifdef WITH_SELINUX

#include "defines.h"

#include <selinux/selinux.h>


static bool selinux_checked = false;
static bool selinux_enabled;

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
int set_selinux_file_context (const char *dst_name)
{
	/*@null@*/security_context_t scontext = NULL;

	if (!selinux_checked) {
		selinux_enabled = is_selinux_enabled () > 0;
		selinux_checked = true;
	}

	if (selinux_enabled) {
		/* Get the default security context for this file */
		if (matchpathcon (dst_name, 0, &scontext) < 0) {
			if (security_getenforce () != 0) {
				return 1;
			}
		}
		/* Set the security context for the next created file */
		if (setfscreatecon (scontext) < 0) {
			if (security_getenforce () != 0) {
				return 1;
			}
		}
		freecon (scontext);
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
		if (setfscreatecon (NULL) != 0) {
			return 1;
		}
	}
	return 0;
}

#else				/* !WITH_SELINUX */
extern int errno;		/* warning: ANSI C forbids an empty source file */
#endif				/* !WITH_SELINUX */
