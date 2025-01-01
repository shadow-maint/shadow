// SPDX-FileCopyrightText: 1990-1994, Julianne Frances Haugh
// SPDX-FileCopyrightText: 1996-1998, Marek Michałkiewicz
// SPDX-FileCopyrightText: 2003-2006, Tomasz Kłoczko
// SPDX-FileCopyrightText: 2008, Nicolas François
// SPDX-FileCopyrightText: 2023-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRDUP_STRDUP_H_
#define SHADOW_INCLUDE_LIB_STRING_STRDUP_STRDUP_H_


#include "config.h"

#include <string.h>

#include "exit_if_null.h"


// xstrdup - exit-on-error string duplicate
#define xstrdup(s)  exit_if_null(strdup(s))


#endif  // include guard
