/*
 * Copyright (c) 1990 - 1994, Julianne Frances Haugh
 * Copyright (c) 1996 - 2000, Marek Michałkiewicz
 * Copyright (c) 2001       , Michał Moskal
 * Copyright (c) 2003 - 2005, Tomasz Kłoczko
 * Copyright (c) 2007 - 2009, Nicolas François
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

#ident "$Id$"

#include <stdio.h>
#include "defines.h"
#include "prototypes.h"
#include "pwio.h"

/*@null@*/ /*@only@*/struct passwd *__pw_dup (const struct passwd *pwent)
{
	struct passwd *pw;

	pw = (struct passwd *) malloc (sizeof *pw);
	if (NULL == pw) {
		return NULL;
	}
	/* The libc might define other fields. They won't be copied. */
	memset (pw, 0, sizeof *pw);
	pw->pw_uid = pwent->pw_uid;
	pw->pw_gid = pwent->pw_gid;
	pw->pw_name = strdup (pwent->pw_name);
	if (NULL == pw->pw_name) {
		free(pw);
		return NULL;
	}
	pw->pw_passwd = strdup (pwent->pw_passwd);
	if (NULL == pw->pw_passwd) {
		free(pw->pw_name);
		free(pw);
		return NULL;
	}
	pw->pw_gecos = strdup (pwent->pw_gecos);
	if (NULL == pw->pw_gecos) {
		free(pw->pw_passwd);
		free(pw->pw_name);
		free(pw);
		return NULL;
	}
	pw->pw_dir = strdup (pwent->pw_dir);
	if (NULL == pw->pw_dir) {
		free(pw->pw_gecos);
		free(pw->pw_passwd);
		free(pw->pw_name);
		free(pw);
		return NULL;
	}
	pw->pw_shell = strdup (pwent->pw_shell);
	if (NULL == pw->pw_shell) {
		free(pw->pw_dir);
		free(pw->pw_gecos);
		free(pw->pw_passwd);
		free(pw->pw_name);
		free(pw);
		return NULL;
	}

	return pw;
}

void pw_free (/*@out@*/ /*@only@*/struct passwd *pwent)
{
	free (pwent->pw_name);
	if (pwent->pw_passwd) {
		memzero (pwent->pw_passwd, strlen (pwent->pw_passwd));
		free (pwent->pw_passwd);
	}
	free (pwent->pw_gecos);
	free (pwent->pw_dir);
	free (pwent->pw_shell);
	free (pwent);
}

