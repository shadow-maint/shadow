// SPDX-FileCopyrightText: 1990-1994, Julianne Frances Haugh
// SPDX-FileCopyrightText: 1996-1998, Marek Michałkiewicz
// SPDX-FileCopyrightText: 2005, Tomasz Kłoczko
// SPDX-FileCopyrightText: 2008-2009, Nicolas François
// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include "shadow/gshadow/getsgent.h"

#include <stddef.h>

#include "shadow/gshadow/fgetsgent.h"
#include "shadow/gshadow/gshadow.h"
#include "shadow/gshadow/setsgent.h"
#include "shadow/gshadow/sgrp.h"


#if defined(SHADOWGRP) && !__has_include(<gshadow.h>)
// get-next shadow group entry
struct sgrp *
getsgent(void)
{
	if (NULL == gshadow) {
		setsgent ();
	}
	return fgetsgent(gshadow);
}
#endif
