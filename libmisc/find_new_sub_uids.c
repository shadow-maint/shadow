/*
 * Copyright (c) 2012 Eric Biederman
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
#include <errno.h>

#include "prototypes.h"
#include "subordinateio.h"
#include "getdef.h"

/*
 * find_new_sub_uids - Find a new unused range of UIDs.
 *
 * If successful, find_new_sub_uids provides a range of unused
 * user IDs in the [SUB_UID_MIN:SUB_UID_MAX] range.
 * 
 * Return 0 on success, -1 if no unused UIDs are available.
 */
int find_new_sub_uids (const char *owner,
		       uid_t *range_start, unsigned long *range_count)
{
	unsigned long min, max;
	unsigned long count;
	uid_t start;

	assert (range_start != NULL);
	assert (range_count != NULL);

	min = getdef_ulong ("SUB_UID_MIN", 100000UL);
	max = getdef_ulong ("SUB_UID_MAX", 600100000UL);
	count = getdef_ulong ("SUB_UID_COUNT", 10000);

	if (min >= max || count >= max || (min + count) >= max) {
		(void) fprintf (stderr,
				_("%s: Invalid configuration: SUB_UID_MIN (%lu),"
				  " SUB_UID_MAX (%lu), SUB_UID_COUNT (%lu)\n"),
			Prog, min, max, count);
		return -1;
	}

	/* Is there a preferred range that works? */
	if ((*range_count != 0) &&
	    (*range_start >= min) &&
	    (((*range_start) + (*range_count) - 1) <= max) &&
	    is_sub_uid_range_free(*range_start, *range_count)) {
		return 0;
	}

	if (max < (min + count)) {
		(void) fprintf (stderr,
				_("%s: Invalid configuration: SUB_UID_MIN (%lu), SUB_UID_MAX (%lu)\n"),
			Prog, min, max);
		return -1;
	}
	start = sub_uid_find_free_range(min, max, count);
	if (start == (uid_t)-1) {
		fprintf (stderr,
		         _("%s: Can't get unique secondary UID range\n"),
		         Prog);
		SYSLOG ((LOG_WARN, "no more available secondary UIDs on the system"));
		return -1;
	}
	*range_start = start;
	*range_count = count;
	return 0;
}

