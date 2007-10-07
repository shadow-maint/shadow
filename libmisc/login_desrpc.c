/* Taken from logdaemon-5.0, only minimal changes.  --marekm */

/************************************************************************
* Copyright 1995 by Wietse Venema.  All rights reserved. Individual files
* may be covered by other copyrights (as noted in the file itself.)
*
* This material was originally written and compiled by Wietse Venema at
* Eindhoven University of Technology, The Netherlands, in 1990, 1991,
* 1992, 1993, 1994 and 1995.
*
* Redistribution and use in source and binary forms are permitted
* provided that this entire copyright notice is duplicated in all such
* copies.  
*
* This software is provided "as is" and without any expressed or implied
* warranties, including, without limitation, the implied warranties of
* merchantibility and fitness for any particular purpose.
************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef DES_RPC
#include "rcsid.h"
RCSID ("$Id: login_desrpc.c,v 1.8 2003/04/22 10:59:22 kloczek Exp $")
#include "defines.h"
    /*
     * Decrypt the user's secret secure RPC key and stores it into the
     * keyserver. Returns 0 if successful, -1 on failure.
     * 
     * Author: Wietse Venema, Eindhoven University of Technology, The Netherlands.
     */
#include <stdio.h>
#include <rpc/rpc.h>
#include <rpc/key_prot.h>
#if !(defined __GLIBC__ && (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ > 0)))
/* these don't seem to be in any header file (libc-5.4.33) */
/* but will be in glibc 2.1 <rpc/auth.h> and <rpc/auth_des.h> */
extern int getnetname (char *);
extern int getsecretkey (const char *, char *, const char *);
extern int key_setsecret (const char *);
#endif

int login_desrpc (const char *passwd)
{
	char netname[MAXNETNAMELEN + 1];
	char secretkey[HEXKEYBYTES + 1];

	if (getnetname (netname) == 0)
		return -1;

	if (getsecretkey (netname, secretkey, passwd) == 0)
		return -1;

	if (secretkey[0] == 0) {
		fprintf (stderr,
			 _
			 ("Password does not decrypt secret key for %s.\n"),
			 netname);
		return -1;
	}
	if (key_setsecret (secretkey) < 0) {
		fprintf (stderr,
			 _
			 ("Could not set %s's secret key: is the keyserv daemon running?\n"),
			 netname);
		return -1;
	}
	return 0;
}
#else
extern int errno;		/* warning: ANSI C forbids an empty source file */
#endif
