/*
 * Copyright (c) 1990 - 1994, Julianne Frances Haugh
 * Copyright (c) 1996 - 2000, Marek Michałkiewicz
 * Copyright (c) 2001       , Michał Moskal
 * Copyright (c) 2005       , Tomasz Kłoczko
 * Copyright (c) 2007 - 2013, Nicolas François
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

#include <config.h>

#ident "$Id$"

#include "prototypes.h"
#include "defines.h"
#include <shadow.h>
#include <stdio.h>
#include "shadowio.h"

/*@null@*/ /*@only@*/struct spwd *__spw_dup (const struct spwd *spent)
{
	struct spwd *sp;

	sp = (struct spwd *) malloc (sizeof *sp);
	if (NULL == sp) {
		return NULL;
	}
	/* The libc might define other fields. They won't be copied. */
	memset (sp, 0, sizeof *sp);
	sp->sp_lstchg = spent->sp_lstchg;
	sp->sp_min    = spent->sp_min;
	sp->sp_max    = spent->sp_max;
	sp->sp_warn   = spent->sp_warn;
	sp->sp_inact  = spent->sp_inact;
	sp->sp_expire = spent->sp_expire;
	sp->sp_flag   = spent->sp_flag;
	/*@-mustfreeonly@*/
	sp->sp_namp   = strdup (spent->sp_namp);
	/*@=mustfreeonly@*/
	if (NULL == sp->sp_namp) {
		free(sp);
		return NULL;
	}
	/*@-mustfreeonly@*/
	sp->sp_pwdp = strdup (spent->sp_pwdp);
	/*@=mustfreeonly@*/
	if (NULL == sp->sp_pwdp) {
		free(sp->sp_namp);
		free(sp);
		return NULL;
	}

	return sp;
}

void spw_free (/*@out@*/ /*@only@*/struct spwd *spent)
{
	if (spent != NULL) {
		free (spent->sp_namp);
		if (NULL != spent->sp_pwdp) {
			memzero (spent->sp_pwdp, strlen (spent->sp_pwdp));
			free (spent->sp_pwdp);
		}
		free (spent);
	}
}

