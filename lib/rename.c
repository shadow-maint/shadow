/*
 * Copyright 1993 - 1994, Julianne Frances Haugh
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
 * 3. Neither the name of Julianne F. Haugh nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY JULIE HAUGH AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL JULIE HAUGH OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <config.h>

#include "rcsid.h"
RCSID("$Id: rename.c,v 1.3 1997/12/07 23:26:57 marekm Exp $")

#include "defines.h"
#include <sys/stat.h>
#include <errno.h>

/*
 * rename - rename a file to another name
 *
 *	rename is provided for systems which do not include the rename()
 *	system call.
 */

int
rename(const char *begin, const char *end)
{
	struct	stat	s1, s2;
	extern	int	errno;
	int	orig_err = errno;

	if (stat (begin, &s1))
		return -1;

	if (stat (end, &s2)) {
		errno = orig_err;
	} else {

		/*
		 * See if this is a cross-device link.  We do this to
		 * insure that the link below has a chance of working.
		 */

		if (s1.st_dev != s2.st_dev) {
			errno = EXDEV;
			return -1;
		}

		/*
		 * See if we can unlink the existing destination
		 * file.  If the unlink works the directory is writable,
		 * so there is no need here to figure that out.
		 */

		if (unlink (end))
			return -1;
	}

	/*
	 * Now just link the original name to the final name.  If there
	 * was no file previously, this link will fail if the target
	 * directory isn't writable.  The unlink will fail if the source
	 * directory isn't writable, but life stinks ...
	 */

	if (link (begin, end) || unlink (begin))
		return -1;

	return 0;
}
