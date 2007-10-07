/*
 * Copyright 1989 - 1994, Julianne Frances Haugh
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
 * 3. Neither the name of Julianne F. Haugh nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY JULIE HAUGH AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL JULIE HAUGH OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <config.h>

#ifdef SHADOWPWD /*{*/
#ifndef HAVE_PUTSPENT

#include "rcsid.h"
RCSID("$Id: putspent.c,v 1.3 1997/12/07 23:26:54 marekm Exp $")

#include <sys/types.h>
#include "prototypes.h"
#include "defines.h"
#include <stdio.h>

int
putspent(const struct spwd *sp, FILE *fp)
{
	int	errors = 0;

	if (! fp || ! sp)
		return -1;

	if (fprintf (fp, "%s:%s:", sp->sp_namp, sp->sp_pwdp) < 0)
		errors++;

	if (sp->sp_lstchg != -1) {
		if (fprintf (fp, "%ld:", sp->sp_lstchg) < 0)
			errors++;
	} else if (putc (':', fp) == EOF)
		errors++;

	if (sp->sp_min != -1) {
		if (fprintf (fp, "%ld:", sp->sp_min) < 0)
			errors++;
	} else if (putc (':', fp) == EOF)
		errors++;

	if (sp->sp_max != -1) {
		if (fprintf (fp, "%ld:", sp->sp_max) < 0)
			errors++;
	} else if (putc (':', fp) == EOF)
		errors++;

	if (sp->sp_warn != -1) {
		if (fprintf (fp, "%ld:", sp->sp_warn) < 0)
			errors++;
	} else if (putc (':', fp) == EOF)
		errors++;

	if (sp->sp_inact != -1) {
		if (fprintf (fp, "%ld:", sp->sp_inact) < 0)
			errors++;
	} else if (putc (':', fp) == EOF)
		errors++;

	if (sp->sp_expire != -1) {
		if (fprintf (fp, "%ld:", sp->sp_expire) < 0)
			errors++;
	} else if (putc (':', fp) == EOF)
		errors++;

	if (sp->sp_flag != -1) {
		if (fprintf (fp, "%ld", sp->sp_flag) < 0)
			errors++;
	}
	if (putc ('\n', fp) == EOF)
		errors++;

	if (errors)
		return -1;
	else
		return 0;
}
#endif
#endif	/*}*/
