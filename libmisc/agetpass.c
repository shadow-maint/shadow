/*
 * Copyright (c) 2022, Alejandro Colomar <alx@kernel.org>
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

#include <limits.h>
#include <readpassphrase.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ident "$Id$"

#include "prototypes.h"


#if !defined(PASS_MAX)
#define PASS_MAX  BUFSIZ
#endif


/*
 * SYNOPSIS
 *	[[gnu::malloc(erase_pass)]]
 *	char *agetpass(const char *prompt);
 *
 *	void erase_pass(char *pass);
 *
 * ARGUMENTS
 *   agetpass()
 *	prompt	String to be printed before reading a password.
 *
 *   erase_pass()
 *	pass	password previously returned by agetpass().
 *
 * DESCRIPTION
 *   agetpass()
 *	This function is very similar to getpass(3).  It has several
 *	advantages compared to getpass(3):
 *
 *	- Instead of using a static buffer, agetpass() allocates memory
 *	  through malloc(3).  This makes the function thread-safe, and
 *	  also reduces the visibility of the buffer.
 *
 *	- agetpass() doesn't call realloc(3) internally.  Some
 *	  implementations of getpass(3), such as glibc, do that, as a
 *	  consequence of calling getline(3).  That's a bug in glibc,
 *	  which allows leaking prefixes of passwords in freed memory.
 *
 *	- agetpass() doesn't overrun the output buffer.  If the input
 *	  password is too long, it simply fails.  Some implementations
 *	  of getpass(3), share the same bug that gets(3) has.
 *
 *	As soon as possible, the password obtained from agetpass() be
 *	erased by calling erase_pass(), to avoid possibly leaking the
 *	password.
 *
 *   erase_pass()
 *	This function first clears the password, by calling
 *	explicit_bzero(3) (or an equivalent call), and then frees the
 *	allocated memory by calling free(3).
 *
 *	NULL is a valid input pointer, and in such a case, this call is
 *	a no-op.
 *
 * RETURN VALUE
 *	agetpass() returns a newly allocated buffer containing the
 *	password on success.  On error, errno is set to indicate the
 *	error, and NULL is returned.
 *
 * ERRORS
 *   agetpass()
 *	This function may fail for any errors that malloc(3) or
 *	readpassphrase(3) may fail, and in addition it may fail for the
 *	following errors:
 *
 *	ENOBUFS
 *		The input password was longer than PASS_MAX.
 *
 * CAVEATS
 *	If a password is passed twice to erase_pass(), the behavior is
 *	undefined.
 */


char *
agetpass(const char *prompt)
{
	char    *pass;
	size_t  len;

	pass = malloc(PASS_MAX);
	if (pass == NULL)
		return NULL;

	if (readpassphrase(prompt, pass, PASS_MAX, RPP_REQUIRE_TTY) == NULL)
		goto fail;

	len = strlen(pass);

	if (len == 0)
		return pass;

	if (pass[len - 1] != '\n') {
		errno = ENOBUFS;
		goto fail;
	}

	pass[len - 1] = '\0';

	return pass;

fail:
	memzero(pass, PASS_MAX);
	free(pass);
	return NULL;
}


void
erase_pass(char *pass)
{
	if (pass == NULL)
		return;
	memzero(pass, PASS_MAX);
	free(pass);
}
