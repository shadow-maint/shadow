// SPDX-FileCopyrightText: 2022-2023, Christian Göttsche <cgzones@googlemail.com>
// SPDX-FileCopyrightText: 2023-2026, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRZERO_STRZERO_H_
#define SHADOW_INCLUDE_LIB_STRING_STRZERO_STRZERO_H_


#include "config.h"

#include <string.h>

#include "memory/memset/memzero.h"


inline char *strzero(char *s);


// strzero - string zero (explicit)
inline char *
strzero(char *s)
{
	return memzero(s, strlen(s));
}


#endif  // include guard
