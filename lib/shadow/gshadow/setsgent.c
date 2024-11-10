// SPDX-FileCopyrightText: 1990-1994, Julianne Frances Haugh
// SPDX-FileCopyrightText: 1996-1998, Marek Michałkiewicz
// SPDX-FileCopyrightText: 2005, Tomasz Kłoczko
// SPDX-FileCopyrightText: 2008-2009, Nicolas François
// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include "shadow/gshadow/setsgent.h"

#include <paths.h>
#include <stddef.h>
#include <stdio.h>

#include "shadow/gshadow/gshadow.h"


#if defined(SHADOWGRP) && !__has_include(<gshadow.h>)
// set-resources-for-working-with shadow group entries
void
setsgent(void)
{
	if (NULL != gshadow) {
		rewind(gshadow);
	} else {
		gshadow = fopen(_PATH_GSHADOW, "re");
	}
}
#endif
