/*
 * Copyright 1990, 1991, John F. Haugh II
 * All rights reserved.
 *
 * Permission is granted to copy and create derivative works for any
 * non-commercial purpose, provided this copyright notice is preserved
 * in all copies of source code, or included in human readable form
 * and conspicuously displayed on all copies of object code or
 * distribution media.
 */

#include <stdio.h>
#ifdef	BSD
#include <strings.h>
#else
#include <string.h>
#endif

#include "shadow.h"

#ifndef	lint
static	char	sccsid[] = "@(#)sppack.c	3.2	08:46:24	12 Sep 1991";
#endif

int	spw_pack (spwd, buf)
struct	spwd	*spwd;
char	*buf;
{
	char	*cp;

	cp = buf;
	strcpy (cp, spwd->sp_namp);
	cp += strlen (cp) + 1;

	strcpy (cp, spwd->sp_pwdp);
	cp += strlen (cp) + 1;

	memcpy (cp, &spwd->sp_min, sizeof spwd->sp_min);
	cp += sizeof spwd->sp_min;

	memcpy (cp, &spwd->sp_max, sizeof spwd->sp_max);
	cp += sizeof spwd->sp_max;

	memcpy (cp, &spwd->sp_lstchg, sizeof spwd->sp_lstchg);
	cp += sizeof spwd->sp_lstchg;

	memcpy (cp, &spwd->sp_warn, sizeof spwd->sp_warn);
	cp += sizeof spwd->sp_warn;

	memcpy (cp, &spwd->sp_inact, sizeof spwd->sp_inact);
	cp += sizeof spwd->sp_inact;

	memcpy (cp, &spwd->sp_expire, sizeof spwd->sp_expire);
	cp += sizeof spwd->sp_expire;

	memcpy (cp, &spwd->sp_flag, sizeof spwd->sp_flag);
	cp += sizeof spwd->sp_flag;

	return cp - buf;
}

int	spw_unpack (buf, len, spwd)
char	*buf;
int	len;
struct	spwd	*spwd;
{
	char	*org = buf;

	spwd->sp_namp = buf;
	buf += strlen (buf) + 1;

	spwd->sp_pwdp = buf;
	buf += strlen (buf) + 1;

	memcpy (&spwd->sp_min, buf, sizeof spwd->sp_min);
	buf += sizeof spwd->sp_min;

	memcpy (&spwd->sp_max, buf, sizeof spwd->sp_max);
	buf += sizeof spwd->sp_max;

	memcpy (&spwd->sp_lstchg, buf, sizeof spwd->sp_lstchg);
	buf += sizeof spwd->sp_lstchg;

	memcpy (&spwd->sp_warn, buf, sizeof spwd->sp_warn);
	buf += sizeof spwd->sp_warn;

	memcpy (&spwd->sp_inact, buf, sizeof spwd->sp_inact);
	buf += sizeof spwd->sp_inact;

	memcpy (&spwd->sp_expire, buf, sizeof spwd->sp_expire);
	buf += sizeof spwd->sp_expire;

	memcpy (&spwd->sp_flag, buf, sizeof spwd->sp_flag);
	buf += sizeof spwd->sp_flag;

	if (buf - org > len)
		return -1;

	return 0;
}
