/*
 * SPDX-FileCopyrightText: 2023, Alejandro Colomar <alx@kernel.org>
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef SHADOW_INCLUDE_LIB_X_H_
#define SHADOW_INCLUDE_LIB_X_H_


#include <config.h>

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"
#include "shadowlog.h"
#include "typetraits.h"


/* Yeh, but.  Remember that bailing out might leave the system in some
   bizarre state.  You really want to put in error checking, then add
   some back-out failure recovery code. -- jfh */


#define is_error(x)                                                           \
({                                                                            \
	static_assert(is_pointer(x)                                           \
	              || is_same_type(x, int)                                 \
	              || is_same_type(x, long)                                \
	              || is_same_type(x, long long), "");                     \
                                                                              \
	_Generic(x,                                                           \
	         int:       x == -1,                                          \
	         long:      x == -1,                                          \
	         long long: x == -1,                                          \
	         default:   x == NULL);                                       \
})


#define x(e_)                                                                 \
({                                                                            \
	__auto_type  ret = e_;                                                \
                                                                              \
	if (is_error(ret)) {                                                  \
		fprintf(log_get_logfd(), _("%s: %s\n"),                       \
		        log_get_progname(), strerror(errno));                 \
		exit(13);                                                     \
	}                                                                     \
                                                                              \
	ret;                                                                  \
})


#endif  // include guard
