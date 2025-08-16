// SPDX-FileCopyrightText: 1988-1994, Julianne Frances Haugh
// SPDX-FileCopyrightText: 1996-1997, Marek Michałkiewicz
// SPDX-FileCopyrightText: 2003-2005, Tomasz Kłoczko
// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_SHADOW_GSHADOW_SGRP_H_
#define SHADOW_INCLUDE_LIB_SHADOW_GSHADOW_SGRP_H_


#include "config.h"


#if __has_include(<gshadow.h>)
# include <gshadow.h>
#else
struct sgrp {
	char *sg_namp;		/* group name */
	char *sg_passwd;	/* group password */
	char **sg_adm;		/* group administrator list */
	char **sg_mem;		/* group membership list */
};
#endif


#endif  // include guard
