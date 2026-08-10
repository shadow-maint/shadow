/*
 * Copyright 1990, John F. Haugh II
 * All rights reserved.
 *
 * Use, duplication, and disclosure prohibited without
 * the express written permission of the author.
 *
 * Duplication is permitted for non-commercial [ profit making ]
 * purposes provided this and other copyright notices remain
 * intact.
 */

#include <stdio.h>
#include <grp.h>
#ifdef	BSD
#include <strings.h>
#else
#include <string.h>
#endif

#ifndef	lint
static	char	sccsid[] = "@(#)grpack.c	3.3	08:45:46	12 Sep 1991";
#endif

int	gr_pack (group, buf)
struct	group	*group;
char	*buf;
{
	char	*cp;
	int	i;

	cp = buf;
	strcpy (cp, group->gr_name);
	cp += strlen (cp) + 1;

	strcpy (cp, group->gr_passwd);
	cp += strlen (cp) + 1;

	memcpy (cp, (char *) &group->gr_gid, sizeof group->gr_gid);
	cp += sizeof group->gr_gid;

	for (i = 0;group->gr_mem[i];i++) {
		strcpy (cp, group->gr_mem[i]);
		cp += strlen (cp) + 1;
	}
	*cp++ = '\0';

	return cp - buf;
}

int	gr_unpack (buf, len, group)
char	*buf;
int	len;
struct	group	*group;
{
	char	*org = buf;
	int	i;

	group->gr_name = buf;
	buf += strlen (buf) + 1;
	if (buf - org > len)
		return -1;

	group->gr_passwd = buf;
	buf += strlen (buf) + 1;
	if (buf - org > len)
		return -1;

	memcpy ((char *) &group->gr_gid, (char *) buf, sizeof group->gr_gid);
	buf += sizeof group->gr_gid;
	if (buf - org > len)
		return -1;

	for (i = 0;*buf && i < 1024;i++) {
		group->gr_mem[i] = buf;
		buf += strlen (buf) + 1;

		if (buf - org > len)
			return -1;
	}
	group->gr_mem[i] = (char *) 0;
	return 0;
}
