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

/*
 * Shadow group security file structure
 */

struct sgrp {
	char *sg_namp;		/* group name */
	char *sg_passwd;	/* group password */
	char **sg_adm;		/* group administrator list */
	char **sg_mem;		/* group membership list */
};

/*
 * Shadow group security file functions.
 */

#include <stdio.h>		/* for FILE */

/*@observer@*//*@null@*/struct sgrp *getsgent (void);
/*@observer@*//*@null@*/struct sgrp *getsgnam (const char *);
/*@observer@*//*@null@*/struct sgrp *sgetsgent (const char *);
/*@observer@*//*@null@*/struct sgrp *fgetsgent (/*@null@*/FILE *);
void setsgent (void);
void endsgent (void);
int putsgent (const struct sgrp *, FILE *);

#define	GSHADOW	"/etc/gshadow"


#endif  // !__has_include(<gshadow.h>)
#endif  // include guard
