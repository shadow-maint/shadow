// SPDX-FileCopyrightText: 1988-1994, Julianne Frances Haugh
// SPDX-FileCopyrightText: 1996-1997, Marek Michałkiewicz
// SPDX-FileCopyrightText: 2003-2005, Tomasz Kłoczko
// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_SHADOW_GSHADOW_PUTSGENT_H_
#define SHADOW_INCLUDE_LIB_SHADOW_GSHADOW_PUTSGENT_H_


#include <config.h>

#include <stdio.h>

#include "gshadow_.h"


#if __has_include(<gshadow.h>)
# include <gshadow.h>
#else
int putsgent(const struct sgrp *sgrp, FILE *fp);
#endif


#endif  // include guard
