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

#include "config.h"
#include <stdio.h>
#include "pwd.h"
#ifdef	BSD
#include <strings.h>
#define	strchr	index
#define	strrchr	rindex
#else
#include <string.h>
#endif

#ifndef	lint
static	char	sccsid[] = "@(#)pwpack.c	3.4	11:50:31	28 Dec 1991";
#endif

/*
 * pw_pack - convert a (struct pwd) to a packed record
 */

int
pw_pack (passwd, buf)
struct	passwd	*passwd;
char	*buf;
{
	char	*cp;

	cp = buf;
	strcpy (cp, passwd->pw_name);
	cp += strlen (cp) + 1;

	strcpy (cp, passwd->pw_passwd);
#ifdef	ATT_AGE
	if (passwd->pw_age[0]) {
		*cp++ = ',';
		strcat (cp, passwd->pw_age);
	}
#endif
	cp += strlen (cp) + 1;

	memcpy (cp, (void *) &passwd->pw_uid, sizeof passwd->pw_uid);
	cp += sizeof passwd->pw_uid;

	memcpy (cp, (void *) &passwd->pw_gid, sizeof passwd->pw_gid);
	cp += sizeof passwd->pw_gid;
#ifdef	BSD_QUOTAS
	memcpy (cp, (void *) &passwd->pw_quota, sizeof passwd->pw_quota);
	cp += sizeof passwd->pw_quota;
#endif
#ifdef	ATT_COMMENT
	if (passwd->pw_comment) {
		strcpy (cp, passwd->pw_comment);
		cp += strlen (cp) + 1;
	} else
		*cp++ = '\0';
#endif
	strcpy (cp, passwd->pw_gecos);
	cp += strlen (cp) + 1;

	strcpy (cp, passwd->pw_dir);
	cp += strlen (cp) + 1;

	strcpy (cp, passwd->pw_shell);
		cp += strlen (cp) + 1;

	return cp - buf;
}

/*
 * pw_unpack - convert a packed (struct pwd) record to a (struct pwd)
 */

int
pw_unpack (buf, len, passwd)
char	*buf;
int	len;
struct	passwd	*passwd;
{
	char	*org = buf;
	char	*cp;

	memset ((void *) passwd, 0, sizeof *passwd);

	passwd->pw_name = buf;
	buf += strlen (buf) + 1;
	if (buf - org > len)
		return -1;

	passwd->pw_passwd = buf;
	buf += strlen (buf) + 1;
	if (buf - org > len)
		return -1;

#ifdef	ATT_AGE
	if (cp = strchr (passwd->pw_passwd, ',')) {
		*cp++ = '\0';
		passwd->pw_age = cp;
	} else
		passwd->pw_age = "";
#endif

	memcpy ((void *) &passwd->pw_uid, (void *) buf, sizeof passwd->pw_uid);
	buf += sizeof passwd->pw_uid;
	if (buf - org > len)
		return -1;

	memcpy ((void *) &passwd->pw_gid, (void *) buf, sizeof passwd->pw_gid);
	buf += sizeof passwd->pw_gid;
	if (buf - org > len)
		return -1;

#ifdef	BSD_QUOTAS
	memcpy ((void *) &passwd->pw_quota, (void *) buf,
		sizeof passwd->pw_quota);
	buf += sizeof passwd->pw_quota;
	if (buf - org > len)
		return -1;
#endif
#ifdef	ATT_COMMENT
	passwd->pw_comment = buf;
	buf += strlen (buf) + 1;
	if (buf - org > len)
		return -1;
#endif
	passwd->pw_gecos = buf;
	buf += strlen (buf) + 1;
	if (buf - org > len)
		return -1;

	passwd->pw_dir = buf;
	buf += strlen (buf) + 1;
	if (buf - org > len)
		return -1;

	passwd->pw_shell = buf;
	buf += strlen (buf) + 1;
	if (buf - org > len)
		return -1;

	return 0;
}
