/*
 * Copyright 1990, John F. Haugh II
 * All rights reserved.
 *
 * Permission is granted to copy and create derivative works for any
 * non-commercial purpose, provided this copyright notice is preserved
 * in all copies of source code, or included in human readable form
 * and conspicuously displayed on all copies of object code or
 * distribution media.
 */

#include <stdio.h>
#include "shadow.h"
#ifdef	BSD
#include <strings.h>
#else
#include <string.h>
#endif

#ifndef	lint
static	char	sccsid[] = "@(#)gspack.c	3.1	09:13:50	13 Dec 1990";
#endif

/*
 * sgr_pack - convert a shadow group structure to a packed
 *	      shadow group record
 *
 *	sgr_pack takes the shadow group structure and packs
 *	the components in a record.  this record will be
 *	unpacked later by sgr_unpack.
 */

int
sgr_pack (sgrp, buf)
struct	sgrp	*sgrp;
char	*buf;
{
	char	*cp;
	int	i;

	/*
	 * The name and password are both easy - append each string
	 * to the buffer.  These are always the first two strings
	 * in a record.
	 */

	cp = buf;
	strcpy (cp, sgrp->sg_name);
	cp += strlen (cp) + 1;

	strcpy (cp, sgrp->sg_passwd);
	cp += strlen (cp) + 1;

	/*
	 * The arrays of administrators and members are slightly
	 * harder.  Each element is appended as a string, with a
	 * final '\0' appended to serve as a blank string.  The
	 * number of elements is not known in advance, so the
	 * entire collection of administrators must be scanned to
	 * find the start of the members.
	 */

	for (i = 0;sgrp->sg_adm[i];i++) {
		strcpy (cp, sgrp->sg_adm[i]);
		cp += strlen (cp) + 1;
	}
	*cp++ = '\0';

	for (i = 0;sgrp->sg_mem[i];i++) {
		strcpy (cp, sgrp->sg_mem[i]);
		cp += strlen (cp) + 1;
	}
	*cp++ = '\0';

	return cp - buf;
}

/*
 * sgr_unpack - convert a packed shadow group record to an
 *	        unpacked record
 *
 *	sgr_unpack converts a record which was packed by sgr_pack
 *	into the normal shadow group structure format.
 */

int
sgr_unpack (buf, len, sgrp)
char	*buf;
int	len;
struct	sgrp	*sgrp;
{
	char	*org = buf;
	int	i;

	/*
	 * The name and password are both easy - they are the first
	 * two strings in the record.
	 */

	sgrp->sg_name = buf;
	buf += strlen (buf) + 1;
	if (buf - org > len)
		return -1;

	sgrp->sg_passwd = buf;
	buf += strlen (buf) + 1;
	if (buf - org > len)
		return -1;

	/*
	 * The administrators and members are slightly more difficult.
	 * The arrays are lists of strings.  Each list is terminated
	 * by a string of length zero.  This string is detected by
	 * looking for an initial character of '\0'.
	 */

	for (i = 0;*buf && i < 1024;i++) {
		sgrp->sg_adm[i] = buf;
		buf += strlen (buf) + 1;

		if (buf - org > len)
			return -1;
	}
	sgrp->sg_adm[i] = (char *) 0;
	if (! *buf)
		buf++;

	for (i = 0;*buf && i < 1024;i++) {
		sgrp->sg_mem[i] = buf;
		buf += strlen (buf) + 1;

		if (buf - org > len)
			return -1;
	}
	sgrp->sg_mem[i] = (char *) 0;

	return 0;
}
