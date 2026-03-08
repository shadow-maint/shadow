// SPDX-FileCopyrightText: 2022-2023, Christian Göttsche <cgzones@googlemail.com>
// SPDX-FileCopyrightText: 2023-2026, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRZERO_STRZERO_H_
#define SHADOW_INCLUDE_LIB_STRING_STRZERO_STRZERO_H_


#include "config.h"

#include <string.h>

#include "memory/memset/memzero.h"


// strzero - string zero (explicit)
#define strzero(s)       ((VQChar_of(s) *) strzero_(s))


inline char *strzero_(volatile char *s);


// strzero - string zero (explicit)
inline char *
strzero_(volatile char *s)
{
	return (char *) memzero(s, strlen(s));
}


#endif  // include guard
