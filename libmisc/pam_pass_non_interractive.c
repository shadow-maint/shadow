/*
 * Copyright (c)        2009, Nicolas François
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

#ident "$Id:$"

#ifdef USE_PAM
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <security/pam_appl.h>
#include "prototypes.h"

/*@null@*/ /*@only@*/static const char *non_interactive_password = NULL;
static int ni_conv (int num_msg,
                    const struct pam_message **msg,
                    struct pam_response **resp,
                    unused void *appdata_ptr);
static struct pam_conv non_interactive_pam_conv = {
	ni_conv,
	NULL
};



static int ni_conv (int num_msg,
                    const struct pam_message **msg,
                    struct pam_response **resp,
                    unused void *appdata_ptr)
{
	struct pam_response *responses;
	int count;

	assert (NULL != non_interactive_password);

	if (num_msg <= 0) {
		return PAM_CONV_ERR;
	}

	responses = (struct pam_response *) calloc ((size_t) num_msg,
	                                            sizeof (*responses));
	if (NULL == responses) {
		return PAM_CONV_ERR;
	}

	for (count=0; count < num_msg; count++) {
		responses[count].resp_retcode = 0;

		switch (msg[count]->msg_style) {
		case PAM_PROMPT_ECHO_ON:
			fprintf (stderr,
			         _("%s: PAM modules requesting echoing are not supported.\n"),
			         Prog);
			goto failed_conversation;
		case PAM_PROMPT_ECHO_OFF:
			responses[count].resp = strdup (non_interactive_password);
			if (NULL == responses[count].resp) {
				goto failed_conversation;
			}
			break;
		case PAM_ERROR_MSG:
			if (   (NULL == msg[count]->msg)
			    || (fprintf (stderr, "%s\n", msg[count]->msg) <0)) {
				goto failed_conversation;
			}
			responses[count].resp = NULL;
			break;
		case PAM_TEXT_INFO:
			if (   (NULL == msg[count]->msg)
			    || (fprintf (stdout, "%s\n", msg[count]->msg) <0)) {
				goto failed_conversation;
			}
			responses[count].resp = NULL;
			break;
		default:
			(void) fprintf (stderr,
			                _("%s: conversation type %d not supported.\n"),
			                Prog, msg[count]->msg_style);
			goto failed_conversation;
		}
	}

	*resp = responses;

	return PAM_SUCCESS;

failed_conversation:
	for (count=0; count < num_msg; count++) {
		if (NULL != responses[count].resp) {
			memset (responses[count].resp, 0,
			        strlen (responses[count].resp));
			free (responses[count].resp);
			responses[count].resp = NULL;
		}
	}

	free (responses);
	*resp = NULL;

	return PAM_CONV_ERR;
}


/*
 * Change non interactively the user's password using PAM.
 *
 * Return 0 on success, 1 on failure.
 */
int do_pam_passwd_non_interractive (const char *pam_service,
                                    const char *username,
                                    const char* password)
{
	pam_handle_t *pamh = NULL;
	int ret;

	ret = pam_start (pam_service, username, &non_interactive_pam_conv, &pamh);
	if (ret != PAM_SUCCESS) {
		fprintf (stderr,
		         _("%s: (user %s) pam_start failure %d\n"),
		         Prog, username, ret);
		return 1;
	}

	non_interactive_password = password;
	ret = pam_chauthtok (pamh, 0);
	if (ret != PAM_SUCCESS) {
		fprintf (stderr,
		         _("%s: (user %s) pam_chauthtok() failed, error:\n"
		           "%s\n"),
		         Prog, username, pam_strerror (pamh, ret));
	}

	(void) pam_end (pamh, PAM_SUCCESS);

	return ((PAM_SUCCESS == ret) ? 0 : 1);
}
#else				/* !USE_PAM */
extern int errno;		/* warning: ANSI C forbids an empty source file */
#endif				/* !USE_PAM */
