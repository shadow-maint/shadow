/*
 * SPDX-FileCopyrightText: 2017, Chris Lamb
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <config.h>

#ident "$Id$"

#include <errno.h>
#include <limits.h>
#include <stdio.h>

#include "atoi/strtou_noneg.h"
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
/*@observer@*/time_t gettime (void)
{
	char  *end;
	char *source_date_epoch;
	time_t fallback;
	unsigned long long epoch;
	FILE *shadow_logfd = log_get_logfd();

	fallback = time (NULL);
	source_date_epoch = shadow_getenv ("SOURCE_DATE_EPOCH");

	if (!source_date_epoch)
		return fallback;

	errno = 0;
	epoch = strtoull_noneg(source_date_epoch, &end, 10);
	if (errno != 0) {
		fprintf (shadow_logfd,
			 _("Environment variable $SOURCE_DATE_EPOCH: strtoull: %s\n"),
			 strerror(errno));
	} else if (end == source_date_epoch) {
		fprintf (shadow_logfd,
			 _("Environment variable $SOURCE_DATE_EPOCH: No digits were found: %s\n"),
			 end);
	} else if (*end != '\0') {
		fprintf (shadow_logfd,
			 _("Environment variable $SOURCE_DATE_EPOCH: Trailing garbage: %s\n"),
			 end);
	} else if (epoch > ULONG_MAX) {
		fprintf (shadow_logfd,
			 _("Environment variable $SOURCE_DATE_EPOCH: value must be smaller than or equal to %lu but was found to be: %llu\n"),
			 ULONG_MAX, epoch);
	} else if ((time_t)epoch > fallback) {
		fprintf (shadow_logfd,
			 _("Environment variable $SOURCE_DATE_EPOCH: value must be smaller than or equal to the current time (%lu) but was found to be: %llu\n"),
			 fallback, epoch);
	} else {
		/* Valid */
		return epoch;
	}

	return fallback;
}
