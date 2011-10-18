/*
 * Copyright (c) 2008 - 2011, Nicolas François
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

#include <assert.h>
#include <stdio.h>

#include "prototypes.h"

/*
 * The cleanup_functions stack.
 */
#define CLEANUP_FUNCTIONS 10

typedef /*@null@*/void * parg_t;

static cleanup_function cleanup_functions[CLEANUP_FUNCTIONS];
static parg_t cleanup_function_args[CLEANUP_FUNCTIONS];
static pid_t cleanup_pid = 0;

/*
 * - Cleanup functions shall not fail.
 * - You should register do_cleanups with atexit.
 * - You should add cleanup functions to the stack with add_cleanup when
 *   an operation is expected to be executed later, and remove it from the
 *   stack with del_cleanup when it has been executed.
 *
 **/

/*
 * do_cleanups - perform the actions stored in the cleanup_functions stack.
 *
 * Cleanup action are not executed on exit of the processes started by the
 * parent (first caller of add_cleanup).
 *
 * It is intended to be used as:
 *     atexit (do_cleanups);
 */
void do_cleanups (void)
{
	unsigned int i;

	/* Make sure there were no overflow */
	assert (NULL == cleanup_functions[CLEANUP_FUNCTIONS-1]);

	if (getpid () != cleanup_pid) {
		return;
	}

	i = CLEANUP_FUNCTIONS;
	do {
		i--;
		if (cleanup_functions[i] != NULL) {
			cleanup_functions[i] (cleanup_function_args[i]);
		}
	} while (i>0);
}

/*
 * add_cleanup - Add a cleanup_function to the cleanup_functions stack.
 */
void add_cleanup (/*@notnull@*/cleanup_function pcf, /*@null@*/void *arg)
{
	unsigned int i;
	assert (NULL != pcf);

	assert (NULL == cleanup_functions[CLEANUP_FUNCTIONS-2]);

	if (0 == cleanup_pid) {
		cleanup_pid = getpid ();
	}

	/* Add the cleanup_function at the end of the stack */
	for (i=0; NULL != cleanup_functions[i]; i++);
	cleanup_functions[i] = pcf;
	cleanup_function_args[i] = arg;
}

/*
 * del_cleanup - Remove a cleanup_function from the cleanup_functions stack.
 */
void del_cleanup (/*@notnull@*/cleanup_function pcf)
{
	unsigned int i;
	assert (NULL != pcf);

	/* Find the pcf cleanup function */
	for (i=0; i<CLEANUP_FUNCTIONS; i++) {
		if (cleanup_functions[i] == pcf) {
			break;
		}
	}

	/* Make sure the cleanup function was found */
	assert (i<CLEANUP_FUNCTIONS);

	/* Move the rest of the cleanup functions */
	for (; i<CLEANUP_FUNCTIONS; i++) {
		/* Make sure the cleanup function was specified only once */
		assert (   (i == (CLEANUP_FUNCTIONS -1))
		        || (cleanup_functions[i+1] != pcf));

		if (i == (CLEANUP_FUNCTIONS -1)) {
			cleanup_functions[i] = NULL;
			cleanup_function_args[i] = NULL;
		} else {
			cleanup_functions[i] = cleanup_functions[i+1];
			cleanup_function_args[i] = cleanup_function_args[i+1];
		}

		/* A NULL indicates the end of the stack */
		if (NULL == cleanup_functions[i]) {
			break;
		}
	}
}

