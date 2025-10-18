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


/*
 * This macro is used for implementing x*() variants of functions that
 * allocate memory, such as xstrdup() for wrapping strdup(3).  The macro
 * returns the input pointer transparently, with the same type, but
 * calls exit(3) if the input is a null pointer (thus, if the allocation
 * failed).
 */
#define exit_if_null(p)                                               \
({                                                                    \
	__auto_type  p_ = p;                                          \
                                                                      \
	exit_if_null_(p_);                                            \
	p_;                                                           \
})


inline void exit_if_null_(void *p);


inline void
exit_if_null_(void *p)
{
	if (p == NULL) {
		fprintf(log_get_logfd(), "%s: %s\n",
		        log_get_progname(), strerror(errno));
		exit(13);
	}
}


#endif  // include guard
