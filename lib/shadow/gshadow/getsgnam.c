// SPDX-FileCopyrightText: 1990-1994, Julianne Frances Haugh
// SPDX-FileCopyrightText: 1996-1998, Marek Michałkiewicz
// SPDX-FileCopyrightText: 2005, Tomasz Kłoczko
// SPDX-FileCopyrightText: 2008-2009, Nicolas François
// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include "shadow/gshadow/getsgnam.h"

#include <stddef.h>
#include <string.h>

#include "defines.h"
#include "shadow/gshadow/setsgent.h"
#include "shadow/gshadow/sgrp.h"


/*
 * getsgnam - get a shadow group entry by name
 */
#if defined(SHADOWGRP) && !__has_include(<gshadow.h>)
// get shadow group entry-by-name
struct sgrp *
getsgnam(const char *name)
{
	struct sgrp *sgrp;

	setsgent ();

	while (NULL != (sgrp = getsgent ())) {
		if (strcmp (name, sgrp->sg_namp) == 0) {
			break;
		}
	}
	return sgrp;
}
#endif
