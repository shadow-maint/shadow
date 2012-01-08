/*
 * Copyright (c) 2010       , Jakub Hrozek <jhrozek@redhat.com>
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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdarg.h>
#include <selinux/selinux.h>
#include <semanage/semanage.h>
#include "prototypes.h"


#ifndef DEFAULT_SERANGE
#define DEFAULT_SERANGE "s0"
#endif


static void semanage_error_callback (unused void *varg,
                                     semanage_handle_t *handle,
                                     const char *fmt, ...)
{
	int ret;
	char * message = NULL;
	va_list ap;


	va_start (ap, fmt);
	ret = vasprintf (&message, fmt, ap);
	va_end (ap);
	if (ret < 0) {
		/* ENOMEM */
		return;
	}

	switch (semanage_msg_get_level (handle)) {
	case SEMANAGE_MSG_ERR:
	case SEMANAGE_MSG_WARN:
		fprintf (stderr, _("[libsemanage]: %s\n"), message);
		break;
	case SEMANAGE_MSG_INFO:
		/* nop */
		break;
	}

	free (message);
}


static semanage_handle_t *semanage_init (void)
{
	int ret;
	semanage_handle_t *handle = NULL;

	handle = semanage_handle_create ();
	if (NULL == handle) {
		fprintf (stderr,
		         _("Cannot create SELinux management handle\n"));
		return NULL;
	}

	semanage_msg_set_callback (handle, semanage_error_callback, NULL);

	ret = semanage_is_managed (handle);
	if (ret != 1) {
		fprintf (stderr, _("SELinux policy not managed\n"));
		goto fail;
	}

	ret = semanage_access_check (handle);
	if (ret < SEMANAGE_CAN_READ) {
		fprintf (stderr, _("Cannot read SELinux policy store\n"));
		goto fail;
	}

	ret = semanage_connect (handle);
	if (ret != 0) {
		fprintf (stderr,
		         _("Cannot establish SELinux management connection\n"));
		goto fail;
	}

	ret = semanage_begin_transaction (handle);
	if (ret != 0) {
		fprintf (stderr, _("Cannot begin SELinux transaction\n"));
		goto fail;
	}

	return handle;

fail:
	semanage_handle_destroy (handle);
	return NULL;
}


static int semanage_user_mod (semanage_handle_t *handle,
                              semanage_seuser_key_t *key,
                              const char *login_name,
                              const char *seuser_name)
{
	int ret;
	semanage_seuser_t *seuser = NULL;

	semanage_seuser_query (handle, key, &seuser);
	if (NULL == seuser) {
		fprintf (stderr,
		         _("Could not query seuser for %s\n"), login_name);
		ret = 1;
		goto done;
	}

	ret = semanage_seuser_set_mlsrange (handle, seuser, DEFAULT_SERANGE);
	if (ret != 0) {
		fprintf (stderr,
		         _("Could not set serange for %s\n"), login_name);
		ret = 1;
		goto done;
	}

	ret = semanage_seuser_set_sename (handle, seuser, seuser_name);
	if (ret != 0) {
		fprintf (stderr,
		         _("Could not set sename for %s\n"),
		         login_name);
		ret = 1;
		goto done;
	}

	ret = semanage_seuser_modify_local (handle, key, seuser);
	if (ret != 0) {
		fprintf (stderr,
		         _("Could not modify login mapping for %s\n"),
		         login_name);
		ret = 1;
		goto done;
	}

	ret = 0;
done:
	semanage_seuser_free (seuser);
	return ret;
}


static int semanage_user_add (semanage_handle_t *handle,
                             semanage_seuser_key_t *key,
                             const char *login_name,
                             const char *seuser_name)
{
	int ret;
	semanage_seuser_t *seuser = NULL;

	ret = semanage_seuser_create (handle, &seuser);
	if (ret != 0) {
		fprintf (stderr,
		         _("Cannot create SELinux login mapping for %s\n"),
		         login_name);
		ret = 1;
		goto done;
	}

	ret = semanage_seuser_set_name (handle, seuser, login_name);
	if (ret != 0) {
		fprintf (stderr, _("Could not set name for %s\n"), login_name);
		ret = 1;
		goto done;
	}

	ret = semanage_seuser_set_mlsrange (handle, seuser, DEFAULT_SERANGE);
	if (ret != 0) {
		fprintf (stderr,
		         _("Could not set serange for %s\n"),
		         login_name);
		ret = 1;
		goto done;
	}

	ret = semanage_seuser_set_sename (handle, seuser, seuser_name);
	if (ret != 0) {
		fprintf (stderr,
		         _("Could not set SELinux user for %s\n"),
		         login_name);
		ret = 1;
		goto done;
	}

	ret = semanage_seuser_modify_local (handle, key, seuser);
	if (ret != 0) {
		fprintf (stderr,
		         _("Could not add login mapping for %s\n"),
		         login_name);
		ret = 1;
		goto done;
	}

	ret = 0;
done:
	semanage_seuser_free (seuser);
	return ret;
}


int set_seuser (const char *login_name, const char *seuser_name)
{
	semanage_handle_t *handle = NULL;
	semanage_seuser_key_t *key = NULL;
	int ret;
	int seuser_exists = 0;

	if (NULL == seuser_name) {
		/* don't care, just let system pick the defaults */
		return 0;
	}

	handle = semanage_init ();
	if (NULL == handle) {
		fprintf (stderr, _("Cannot init SELinux management\n"));
		ret = 1;
		goto done;
	}

	ret = semanage_seuser_key_create (handle, login_name, &key);
	if (ret != 0) {
		fprintf (stderr, _("Cannot create SELinux user key\n"));
		ret = 1;
		goto done;
	}

	ret = semanage_seuser_exists (handle, key, &seuser_exists);
	if (ret < 0) {
		fprintf (stderr, _("Cannot verify the SELinux user\n"));
		ret = 1;
		goto done;
	}

	if (0 != seuser_exists) {
		ret = semanage_user_mod (handle, key, login_name, seuser_name);
		if (ret != 0) {
			fprintf (stderr,
			         _("Cannot modify SELinux user mapping\n"));
			ret = 1;
			goto done;
		}
	} else {
		ret = semanage_user_add (handle, key, login_name, seuser_name);
		if (ret != 0) {
			fprintf (stderr,
			         _("Cannot add SELinux user mapping\n"));
			ret = 1;
			goto done;
		}
	}

	ret = semanage_commit (handle);
	if (ret < 0) {
		fprintf (stderr, _("Cannot commit SELinux transaction\n"));
		ret = 1;
		goto done;
	}

	ret = 0;

done:
	semanage_seuser_key_free (key);
	semanage_handle_destroy (handle);
	return ret;
}


int del_seuser (const char *login_name)
{
	semanage_handle_t *handle = NULL;
	semanage_seuser_key_t *key = NULL;
	int ret;
	int exists = 0;

	handle = semanage_init ();
	if (NULL == handle) {
		fprintf (stderr, _("Cannot init SELinux management\n"));
		ret = 1;
		goto done;
	}

	ret = semanage_seuser_key_create (handle, login_name, &key);
	if (ret != 0) {
		fprintf (stderr, _("Cannot create SELinux user key\n"));
		ret = 1;
		goto done;
	}

	ret = semanage_seuser_exists (handle, key, &exists);
	if (ret < 0) {
		fprintf (stderr, _("Cannot verify the SELinux user\n"));
		ret = 1;
		goto done;
	}

	if (0 == exists) {
		fprintf (stderr,
		         _("Login mapping for %s is not defined, OK if default mapping was used\n"), 
		         login_name);
		ret = 0;  /* probably default mapping */
		goto done;
	}

	ret = semanage_seuser_exists_local (handle, key, &exists);
	if (ret < 0) {
		fprintf (stderr, _("Cannot verify the SELinux user\n"));
		ret = 1;
		goto done;
	}

	if (0 == exists) {
		fprintf (stderr,
		         _("Login mapping for %s is defined in policy, cannot be deleted\n"), 
		         login_name);
		ret = 0; /* Login mapping defined in policy can't be deleted */
		goto done;
	}

	ret = semanage_seuser_del_local (handle, key);
	if (ret != 0) {
		fprintf (stderr,
		         _("Could not delete login mapping for %s"),
		         login_name);
		ret = 1;
		goto done;
	}

	ret = semanage_commit (handle);
	if (ret < 0) {
		fprintf (stderr, _("Cannot commit SELinux transaction\n"));
		ret = 1;
		goto done;
	}

	ret = 0;
done:
	semanage_handle_destroy (handle);
	return ret;
}
#else				/* !WITH_SELINUX */
extern int errno;		/* warning: ANSI C forbids an empty source file */
#endif				/* !WITH_SELINUX */
