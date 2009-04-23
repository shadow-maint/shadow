/*
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

/*
 * According to the Linux-PAM documentation:
 *
 *  4.1. Care about standard library calls
 *
 *  In general, writers of authorization-granting applications should
 *  assume that each module is likely to call any or all 'libc' functions.
 *  For 'libc' functions that return pointers to static/dynamically
 *  allocated structures (ie.  the library allocates the memory and the
 *  user is not expected to 'free()' it) any module call to this function
 *  is likely to corrupt a pointer previously obtained by the application.
 *  The application programmer should either re-call such a 'libc'
 *  function after a call to the Linux-PAM library, or copy the structure
 *  contents to some safe area of memory before passing control to the
 *  Linux-PAM library.
 *
 *  Two important function classes that fall into this category are
 *  getpwnam(3) and syslog(3).
 *
 * This file provide wrapper to the getpwnam or getpwnam_r functions.
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "prototypes.h"

#define XFUNCTION_NAME XPREFIX (FUNCTION_NAME)
#define XPREFIX(name) XPREFIX1 (name)
#define XPREFIX1(name) x##name
#define REENTRANT_NAME APPEND_R (FUNCTION_NAME)
#define APPEND_R(name) APPEND_R1 (name)
#define APPEND_R1(name) name##_r
#define STRINGIZE(name) STRINGIZE1 (name)
#define STRINGIZE1(name) #name

/*@null@*/ /*@only@*/LOOKUP_TYPE *XFUNCTION_NAME (ARG_TYPE ARG_NAME)
{
#if HAVE_FUNCTION_R
	LOOKUP_TYPE *result=NULL;
	char *buffer=NULL;
	/* we have to start with something */
	size_t length = 0x100;

	result = malloc(sizeof(LOOKUP_TYPE));
	if (NULL == result) {
		fprintf (stderr, _("%s: out of memory\n"),
		         "x" STRINGIZE(FUNCTION_NAME));
		exit (13);
	}

	do {
		int status;
		LOOKUP_TYPE *resbuf = NULL;
		buffer = (char *)realloc (buffer, length);
		if (NULL == buffer) {
			fprintf (stderr, _("%s: out of memory\n"),
			         "x" STRINGIZE(FUNCTION_NAME));
			exit (13);
		}
		errno = 0;
		status = REENTRANT_NAME(ARG_NAME, result, buffer,
		                        length, &resbuf);
		if ((0 ==status) && (resbuf == result)) {
			/* Build a result structure that can be freed by
			 * the shadow *_free functions. */
			LOOKUP_TYPE *ret_result = DUP_FUNCTION(result);
			free(buffer);
			free(result);
			return ret_result;
		}

		if (ERANGE != errno) {
			free (buffer);
			free (result);
			return NULL;
		}

		length *= 4;
	} while (length < MAX_LENGTH);

	free(buffer);
	free(result);
	return NULL;

#else /* !HAVE_FUNCTION_R */

	/* No reentrant function.
	 * Duplicate the structure to avoid other call to overwrite it.
	 *
	 * We should also restore the initial structure. But that would be
	 * overkill.
	 */
	LOOKUP_TYPE *result = FUNCTION_NAME(ARG_NAME);

	if (result) {
		result = DUP_FUNCTION(result);
		if (NULL == result) {
			fprintf (stderr, _("%s: out of memory\n"),
			         "x" STRINGIZE(FUNCTION_NAME));
			exit (13);
		}
	}

	return result;
#endif
}

