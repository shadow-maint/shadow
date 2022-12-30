/*
 * SPDX-FileCopyrightText: 2007 - 2009, Nicolas François
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
 * This file provides wrapper to the getpwnam or getpwnam_r functions.
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "prototypes.h"
#include "shadowlog.h"

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
		fprintf (log_get_logfd(), _("%s: out of memory\n"),
		         "x" STRINGIZE(FUNCTION_NAME));
		exit (13);
	}

	while (true) {
		int status;
		LOOKUP_TYPE *resbuf = NULL;
		buffer = (char *)realloc (buffer, length);
		if (NULL == buffer) {
			fprintf (log_get_logfd(), _("%s: out of memory\n"),
			         "x" STRINGIZE(FUNCTION_NAME));
			exit (13);
		}
		status = REENTRANT_NAME(ARG_NAME, result, buffer,
		                        length, &resbuf);
		if ((0 == status) && (resbuf == result)) {
			/* Build a result structure that can be freed by
			 * the shadow *_free functions. */
			LOOKUP_TYPE *ret_result = DUP_FUNCTION(result);
			free(buffer);
			free(result);
			return ret_result;
		}

		if (ERANGE != status) {
			free (buffer);
			free (result);
			return NULL;
		}

		if (length <= ((size_t)-1 / 4)) {
			length *= 4;
		} else if (length == (size_t) -1) {
			break;
		} else {
			length = (size_t) -1;
		}
	}

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
			fprintf (log_get_logfd(), _("%s: out of memory\n"),
			         "x" STRINGIZE(FUNCTION_NAME));
			exit (13);
		}
	}

	return result;
#endif
}

