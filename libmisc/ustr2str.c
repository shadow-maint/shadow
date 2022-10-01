/*
 * Copyright (c) 2022, Alejandro Colomar <alx.manpages@gmail.com>
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

#include <stddef.h>
#include <string.h>

#ident "$Id$"

#include "prototypes.h"


/*
 * SYNOPSIS
 *	void ustr2str(char dst[restrict .n+1], const u_char src[restrict .n],
 *	              size_t n);
 *
 * ARGUMENTS
 *	dst	Pointer to the first byte of the destination buffer.
 *
 *	src	Pointer to the first byte of the source unterminated string.
 *
 *	n	Size of 'src'.
 *
 * DESCRIPTION
 *	Copy a string from the source unterminated string, into a
 *	NUL-terminated string in the destination buffer.
 *
 * CAVEATS
 *	If the destination buffer is not wider than the source buffer
 *	at least by 1 byte, the behavior is undefined.
 *
 *	Use of this function normally indicates a problem in the design
 *	of the strings, since normally it's better to guarantee that all
 *	strings are properly terminated.  The main use for this function
 *	is to interface with some standard buffers, such as those
 *	defined in utmp(7), which for historical reasons are not
 *	guaranteed to be terminated.
 *
 *	Other valid uses are in projects where, for performance reasons, it's
 *	useful to have strings that overlap (thus defining a string by a
 *	pointer and a length).  In such cases, it's necessary to use distinct
 *	types for strings and unterminated strings, so that the compiler can
 *	enforce that we don't mix them accidentally.  This function has been
 *	designed with that use case in mind, and uses u_char* for the
 *	u_nterminated string.
 *
 * EXAMPLES
 *	u_char  src[10] = "0123456789";  // not NUL-terminated
 *	char    dst[sizeof(src) + 1];
 *
 *	static_assert(nitems(src) < nitems(dst))
 *	ustr2str(dst, src, lengthof(src));
 */

void
ustr2str(char *restrict dst, const u_char *restrict src, size_t n)
{
	memcpy(dst, src, n);
	dst[n] = '\0';
}
