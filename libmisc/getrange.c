/*
 * Copyright (c) 2008       , Nicolas François
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

#ident "$Id: $"

#include <ctype.h>
#include <stdlib.h>

#include "defines.h"
#include "prototypes.h"

/*
 * Parse a range and indicate if the range is valid.
 * Valid ranges are in the form:
 *     <long>          -> min=max=long         has_min  has_max
 *     -<long>         -> max=long            !has_min  has_max
 *     <long>-         -> min=long             has_min !has_max
 *     <long1>-<long2> -> min=long1 max=long2  has_min  has_max
 *
 * If the range is valid, getrange returns 1.
 * If the range is not valid, getrange returns 0.
 */
int getrange (char *range,
              unsigned long *min, bool *has_min,
              unsigned long *max, bool *has_max)
{
	char *endptr;
	unsigned long n;

	if (NULL == range) {
		return 0;
	}

	if ('-' == range[0]) {
		if (!isdigit(range[1])) {
			/* invalid */
			return 0;
		}
		errno = 0;
		n = strtoul (&range[1], &endptr, 10);
		if (('\0' != *endptr) || (ERANGE == errno)) {
			/* invalid */
			return 0;
		}
		/* -<long> */
		*has_min = false;
		*has_max = true;
		*max = n;
	} else {
		errno = 0;
		n = strtoul (range, &endptr, 10);
		if (ERANGE == errno) {
			/* invalid */
			return 0;
		}
		switch (*endptr) {
			case '\0':
				/* <long> */
				*has_min = true;
				*has_max = true;
				*min = n;
				*max = n;
				break;
			case '-':
				endptr++;
				if ('\0' == *endptr) {
					/* <long>- */
					*has_min = true;
					*has_max = false;
					*min = n;
				} else if (!isdigit (*endptr)) {
					/* invalid */
					return 0;
				} else {
					*has_min = true;
					*min = n;
					errno = 0;
					n = strtoul (endptr, &endptr, 10);
					if (   ('\0' != *endptr)
					    || (ERANGE == errno)) {
						/* invalid */
						return 0;
					}
					/* <long>-<long> */
					*has_max = true;
					*max = n;
				}
				break;
			default:
				return 0;
		}
	}

	return 1;
}

