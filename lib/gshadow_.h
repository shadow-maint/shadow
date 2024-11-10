// SPDX-FileCopyrightText: 1988-1994, Julianne Frances Haugh
// SPDX-FileCopyrightText: 1996-1997, Marek Michałkiewicz
// SPDX-FileCopyrightText: 2003-2005, Tomasz Kłoczko
// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_GSHADOW__H_
#define SHADOW_INCLUDE_LIB_GSHADOW__H_


#if __has_include(<gshadow.h>)
# include <gshadow.h>
#else


#include <config.h>

#include <stdio.h>		/* for FILE */

#include "shadow/gshadow/sgrp.h"


/*@observer@*//*@null@*/struct sgrp *getsgent (void);
/*@observer@*//*@null@*/struct sgrp *getsgnam (const char *);
/*@observer@*//*@null@*/struct sgrp *sgetsgent (const char *);
/*@observer@*//*@null@*/struct sgrp *fgetsgent (/*@null@*/FILE *);

#define	GSHADOW	"/etc/gshadow"


#endif  // !__has_include(<gshadow.h>)
#endif  // include guard
