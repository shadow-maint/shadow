// SPDX-FileCopyrightText: 2023-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_X_H_
#define SHADOW_INCLUDE_LIB_X_H_


#include "config.h"

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "shadowlog.h"


#define X(p)  ((typeof(p)) x_(p))


inline void *x_(void *p);


inline void *
x_(void *p)
{
	if (p == NULL) {
		fprintf(log_get_logfd(), "%s: %s\n",
		        log_get_progname(), strerror(errno));
		exit(13);
	}

	return p;
}


#endif  // include guard
