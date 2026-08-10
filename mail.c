/*
 * Copyright 1989, 1990, 1991, John F. Haugh II
 * All rights reserved.
 *
 * Permission is granted to copy and create derivative works for any
 * non-commercial purpose, provided this copyright notice is preserved
 * in all copies of source code, or included in human readable form
 * and conspicuously displayed on all copies of object code or
 * distribution media.
 */

#include <sys/types.h>
#include <sys/stat.h>

#ifndef	BSD
#include <string.h>
#include <memory.h>
#else
#include <strings.h>
#define	strchr	index
#define	strrchr	rindex
#endif

#include "config.h"

#ifndef	lint
static	char	sccsid[] = "@(#)mail.c	3.3	07:43:42	17 Sep 1991";
#endif

extern	char	*getenv();
extern	int	getdef_bool();

void	mailcheck ()
{
	struct	stat	statbuf;
	char	*mailbox;

	if (! getdef_bool("MAIL_CHECK_ENAB"))
		return;
	if (! (mailbox = getenv ("MAIL")))
		return;

	if (stat (mailbox, &statbuf) == -1 || statbuf.st_size == 0)
		puts ("No mail.");
	else if (statbuf.st_atime > statbuf.st_mtime)
		puts ("You have mail.");
	else
		puts ("You have new mail.");
}
