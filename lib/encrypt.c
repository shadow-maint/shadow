/*
 * Copyright (c) 1990 - 1993, Julianne Frances Haugh
 * Copyright (c) 1996 - 2000, Marek Michałkiewicz
 * Copyright (c) 2005       , Tomasz Kłoczko
 * Copyright (c) 2007 - 2008, Nicolas François
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

#include <unistd.h>
#include <stdio.h>

#include "prototypes.h"
#include "defines.h"

char *pw_encrypt (const char *clear, const char *salt)
{
	static char cipher[128];
	char *cp;

	cp = crypt (clear, salt);
	if (!cp) {
		/*
		 * Single Unix Spec: crypt() may return a null pointer,
		 * and set errno to indicate an error.  The caller doesn't
		 * expect us to return NULL, so...
		 */
		perror ("crypt");
		exit (1);
	}

	/* The GNU crypt does not return NULL if the algorithm is not
	 * supported, and return a DES encrypted password. */
	if (salt && salt[0] == '$' && strlen (cp) <= 13)
	{
		const char *method;
		switch (salt[1])
		{
			case '1':
				method = "MD5";
				break;
			case '5':
				method = "SHA256";
				break;
			case '6':
				method = "SHA512";
				break;
			default:
			{
				static char nummethod[4] = "$x$";
				nummethod[1] = salt[1];
				method = &nummethod[0];
			}
		}
		fprintf (stderr,
			 _("crypt method not supported by libcrypt? (%s)\n"),
			  method);
		exit (1);
	}

	if (strlen (cp) != 13)
		return cp;	/* nonstandard crypt() in libc, better bail out */
	strcpy (cipher, cp);

	return cipher;
}
