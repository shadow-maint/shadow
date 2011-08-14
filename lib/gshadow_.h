/*
 * Copyright (c) 1988 - 1994, Julianne Frances Haugh
 * Copyright (c) 1996 - 1997, Marek Michałkiewicz
 * Copyright (c) 2003 - 2005, Tomasz Kłoczko
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the copyright holders or contributors may not be used to
 *    endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *	$Id$
 */

#ifndef	_H_GSHADOW
#define	_H_GSHADOW

/*
 * Shadow group security file structure
 */

struct sgrp {
	char *sg_name;		/* group name */
	char *sg_passwd;	/* group password */
	char **sg_adm;		/* group administrator list */
	char **sg_mem;		/* group membership list */
};

/*
 * Shadow group security file functions.
 */

#include <stdio.h>		/* for FILE */

#if __STDC__
/*@observer@*//*@null@*/struct sgrp *getsgent (void);
/*@observer@*//*@null@*/struct sgrp *getsgnam (const char *);
/*@observer@*//*@null@*/struct sgrp *sgetsgent (const char *);
/*@observer@*//*@null@*/struct sgrp *fgetsgent (/*@null@*/FILE *);
void setsgent (void);
void endsgent (void);
int putsgent (const struct sgrp *, FILE *);
#else
/*@observer@*//*@null@*/struct sgrp *getsgent ();
/*@observer@*//*@null@*/struct sgrp *getsgnam ();
/*@observer@*//*@null@*/struct sgrp *sgetsgent ();
/*@observer@*//*@null@*/struct sgrp *fgetsgent ();
void setsgent ();
void endsgent ();
int putsgent ();
#endif

#define	GSHADOW	"/etc/gshadow"
#endif				/* ifndef _H_GSHADOW */
