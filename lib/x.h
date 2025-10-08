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


#define X(p)                                                          \
({                                                                    \
	__auto_type  p_ = p;                                          \
                                                                      \
	exit_if_null(p_);                                             \
	p_;                                                           \
})


inline void exit_if_null(void *p);


inline void
exit_if_null(void *p)
{
	if (p == NULL) {
		fprintf(log_get_logfd(), "%s: %s\n",
		        log_get_progname(), strerror(errno));
		exit(13);
	}
}


#endif  // include guard
