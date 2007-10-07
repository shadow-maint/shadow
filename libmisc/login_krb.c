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

#ifdef KERBEROS
#include "rcsid.h"
RCSID ("$Id: login_krb.c,v 1.4 2003/04/22 10:59:22 kloczek Exp $")
#include <krb.h>
    /*
     * Do an equivalent to kinit here. We need to do the kinit before trying to
     * cd to the home directory, because it might be on a remote filesystem that
     * uses Kerberos authentication. We also need to do this after we've
     * setuid() to the user, or krb_get_pw_in_tkt() won't know where to put the
     * ticket.
     * 
     * We don't really care about whether or not it succeeds; if it fails, we'll
     * just carry on bravely.
     * 
     * NB: we assume: local realm, same username and password as supplied to login.
     * 
     * Security note: if pp is NULL, login doesn't have the password. This is
     * common when it's called by rlogind. Since this is almost always a remote
     * connection, we don't want to risk asking for the password by supplying a
     * NULL pp to krb_get_pw_in_tkt(), because somebody could be listening. So
     * we'll just forget the whole thing.  -jdd
     */
int login_kerberos (const char *username, const char *password)
{
	char realm[REALM_SZ];

	(void) krb_get_lrealm (realm, 1);
	if (password != 0)
		(void) krb_get_pw_in_tkt (username, "", realm, "krbtgt",
					  realm, DEFAULT_TKT_LIFE,
					  password);
}
#else
extern int errno;		/* warning: ANSI C forbids an empty source file */
#endif				/* KERBEROS */
