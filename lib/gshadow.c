/*
 * SPDX-FileCopyrightText: 1990 - 1994, Julianne Frances Haugh
 * SPDX-FileCopyrightText: 1996 - 1998, Marek Michałkiewicz
 * SPDX-FileCopyrightText: 2005       , Tomasz Kłoczko
 * SPDX-FileCopyrightText: 2008 - 2009, Nicolas François
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <config.h>

#if defined(SHADOWGRP) && !__has_include(<gshadow.h>)

#ident "$Id$"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "alloc/malloc.h"
#include "alloc/realloc.h"
#include "alloc/x/xmalloc.h"
#include "defines.h"
#include "prototypes.h"
#include "shadow/gshadow/fgetsgent.h"
#include "shadow/gshadow/gshadow.h"
#include "shadow/gshadow/setsgent.h"
#include "shadow/gshadow/sgrp.h"
#include "string/strchr/strchrcnt.h"
#include "string/strcmp/streq.h"
#include "string/strtok/stpsep.h"


/*
 * getsgent - get a single shadow group entry
 */

/*@observer@*//*@null@*/struct sgrp *getsgent (void)
{
	if (NULL == gshadow) {
		setsgent ();
	}
	return fgetsgent(gshadow);
}

/*
 * getsgnam - get a shadow group entry by name
 */

/*@observer@*//*@null@*/struct sgrp *getsgnam (const char *name)
{
	struct sgrp *sgrp;

	setsgent ();

	while ((sgrp = getsgent ()) != NULL) {
		if (streq(name, sgrp->sg_namp)) {
			break;
		}
	}
	return sgrp;
}
#else
extern int ISO_C_forbids_an_empty_translation_unit;
#endif  // !SHADOWGRP
