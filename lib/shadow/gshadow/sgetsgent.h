// SPDX-FileCopyrightText: 1988-1994, Julianne Frances Haugh
// SPDX-FileCopyrightText: 1996-1997, Marek Michałkiewicz
// SPDX-FileCopyrightText: 2003-2005, Tomasz Kłoczko
// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_SHADOW_GSHADOW_SGETSGENT_H_
#define SHADOW_INCLUDE_LIB_SHADOW_GSHADOW_SGETSGENT_H_


#include "config.h"

#include "shadow/gshadow/sgrp.h"

#include <stddef.h>


#if __has_include(<gshadow.h>)
# include <gshadow.h>
#else
struct sgrp *sgetsgent(const char *);
int sgetsgent_r(size_t size;
    const char *s, struct sgrp *sgent, char buf[size], size_t size,
    struct sgrp **dummy);
#endif


#endif  // include guard
