// SPDX-FileCopyrightText: 2017, Chris Lamb
// SPDX-FileCopyrightText: 2023-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include "time/date.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>

#include "atoi/a2i.h"
#include "defines.h"
#include "prototypes.h"
#include "shadowlog.h"
#include "string/strerrno.h"


extern inline long date_or_SDE(void);


// time(2) or SOURCE_DATE_EPOCH
time_t
time_or_SDE(void)
{
	char    *env;
	FILE    *shadow_logfd = log_get_logfd();
	time_t  t, sde;

	t = time(NULL);

	env = shadow_getenv("SOURCE_DATE_EPOCH");
	if (env == NULL)
		return t;

	if (a2i(time_t, &sde, env, NULL, 10, 0, t) == -1) {
		fprintf(shadow_logfd,
		        _("Environment variable $SOURCE_DATE_EPOCH: a2i(\"%s\"): %s"),
		        env, strerrno());
		return t;
	}

	return sde;
}
