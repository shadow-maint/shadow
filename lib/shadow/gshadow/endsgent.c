// SPDX-FileCopyrightText: 1990-1994, Julianne Frances Haugh
// SPDX-FileCopyrightText: 1996-1998, Marek Michałkiewicz
// SPDX-FileCopyrightText: 2005, Tomasz Kłoczko
// SPDX-FileCopyrightText: 2008-2009, Nicolas François
// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include "shadow/gshadow/endsgent.h"

#include <stddef.h>
#include <stdio.h>

#include "shadow/gshadow/gshadow.h"


#if defined(SHADOWGRP) && !__has_include(<gshadow.h>)
// end-working-with shadow group entries
void
endsgent(void)
{
	if (NULL != gshadow) {
		fclose(gshadow);
	}

	gshadow = NULL;
}
#endif
