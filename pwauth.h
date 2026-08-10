/*
 * Copyright 1992, 1993, John F. Haugh II
 * All rights reserved.
 *
 * Permission is granted to copy and create derivative works for any
 * non-commercial purpose, provided this copyright notice is preserved
 * in all copies of source code, or included in human readable form
 * and conspicuously displayed on all copies of object code or
 * distribution media.
 *
 * This software is provided on an AS-IS basis and the author makes
 * not warrantee of any kind.
 *
 *	@(#)pwauth.h	3.2	17:40:49	01 May 1993
 */

#if	__STDC__
int	pw_auth (char * program, char * user, int flag, char * input);
#else
int	pw_auth ();
#endif

/*
 * Local access
 */

#define	PW_SU		1
#define	PW_LOGIN	2

/*
 * Administrative functions
 */

#define	PW_ADD		101
#define	PW_CHANGE	102
#define	PW_DELETE	103

/*
 * Network access
 */

#define	PW_TELNET	201
#define	PW_RLOGIN	202
#define	PW_FTP		203
#define	PW_REXEC	204
