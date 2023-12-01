/*
 * SPDX-FileCopyrightText: 2017, Chris Lamb
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <config.h>

#ident "$Id$"

#include <inttypes.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "atoi/strtoi.h"
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
	int     status;
	char    *source_date_epoch;
	time_t  fallback, epoch;

	fallback = time (NULL);
	source_date_epoch = shadow_getenv ("SOURCE_DATE_EPOCH");

	if (!source_date_epoch)
		return fallback;

	epoch = strton(source_date_epoch, NULL, 10, 0, fallback, &status, time_t);
	if (status != 0) {
		fprintf(log_get_logfd(),
		        _("Environment variable $SOURCE_DATE_EPOCH: strtoi(\"%s\"): %s\n"),
			source_date_epoch, strerror(status));
		return fallback;
	}

	return epoch;
}
