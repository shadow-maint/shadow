/*
 * Copyright (c) 1992 - 1994, Julianne Frances Haugh
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

/*
 * Common code for yes/no prompting
 *
 * Used by pwck.c and grpck.c
 */

#include <config.h>

#ident "$Id$"

#include <stdio.h>
#include "prototypes.h"

/*
 * yes_or_no - get answer to question from the user
 *
 *	It returns false if no.
 *
 *	If the read_only flag is set, it will print No, and will return
 *	false.
 */
bool yes_or_no (bool read_only)
{
	char buf[80];

	/*
	 * In read-only mode all questions are answered "no".
	 */
	if (read_only) {
		(void) puts (_("No"));
		return false;
	}

	/*
	 * Typically, there's a prompt on stdout, sometimes unflushed.
	 */
	(void) fflush (stdout);

	/*
	 * Get a line and see what the first character is.
	 */
	/* TODO: use gettext */
	if (fgets (buf, (int) sizeof buf, stdin) == buf) {
		return buf[0] == 'y' || buf[0] == 'Y';
	}

	return false;
}

