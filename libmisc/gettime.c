/*
 * Copyright (c) 2017, Chris Lamb
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

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include "defines.h"
#include "prototypes.h"
#include "shadowlog.h"

/*
 * gettime() returns the time as the number of seconds since the Epoch
 *
 * Like time(), gettime() returns the time as the number of seconds since the
 * Epoch, 1970-01-01 00:00:00 +0000 (UTC), except that if the SOURCE_DATE_EPOCH
 * environment variable is exported it will use that instead.
 */
/*@observer@*/time_t gettime ()
{
	char *endptr;
	char *source_date_epoch;
	time_t fallback;
	unsigned long long epoch;
	FILE *shadow_logfd = log_get_logfd();

	fallback = time (NULL);
	source_date_epoch = shadow_getenv ("SOURCE_DATE_EPOCH");

	if (!source_date_epoch)
		return fallback;

	errno = 0;
	epoch = strtoull (source_date_epoch, &endptr, 10);
	if ((errno == ERANGE && (epoch == ULLONG_MAX || epoch == 0))
			|| (errno != 0 && epoch == 0)) {
		fprintf (shadow_logfd,
			 _("Environment variable $SOURCE_DATE_EPOCH: strtoull: %s\n"),
			 strerror(errno));
	} else if (endptr == source_date_epoch) {
		fprintf (shadow_logfd,
			 _("Environment variable $SOURCE_DATE_EPOCH: No digits were found: %s\n"),
			 endptr);
	} else if (*endptr != '\0') {
		fprintf (shadow_logfd,
			 _("Environment variable $SOURCE_DATE_EPOCH: Trailing garbage: %s\n"),
			 endptr);
	} else if (epoch > ULONG_MAX) {
		fprintf (shadow_logfd,
			 _("Environment variable $SOURCE_DATE_EPOCH: value must be smaller than or equal to %lu but was found to be: %llu\n"),
			 ULONG_MAX, epoch);
	} else if (epoch > fallback) {
		fprintf (shadow_logfd,
			 _("Environment variable $SOURCE_DATE_EPOCH: value must be smaller than or equal to the current time (%lu) but was found to be: %llu\n"),
			 fallback, epoch);
	} else {
		/* Valid */
		return (time_t)epoch;
	}

	return fallback;
}
