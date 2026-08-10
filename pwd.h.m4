/*
 * Copyright 1990, 1991, 1992, John F. Haugh II and Steve Simmons
 * All rights reserved.
 *
 * Permission is granted to copy and create derivative works for any
 * non-commercial purpose, provided this copyright notice is preserved
 * in all copies of source code, or included in human readable form
 * and conspicuously displayed on all copies of object code or
 * distribution media.
 *
 * This software is provided on an AS-IS basis and the author makes
 * no warrantee of any kind.
 */

/*
 * Standard definitions for password files.  This is an independant
 * reimplementation of the definitions used by AT&T, BSD, and POSIX.
 * It is not derived from any of those sources.  Note that it can be
 * site-defined to have non-POSIX features as well.  Ideally this file
 * is simply replaced by the standard system supplied /usr/include/pwd.h
 * file.
 *
 *	@(#)pwd.h.m4	3.4.1.2	08:07:12	19 Jul 1993
 */

#ifndef	PWD_H
#define	PWD_H

#ifdef	M_XENIX
typedef int uid_t;
typedef int gid_t;
#endif

#if defined(SUN) || defined(SUN4)
#include <sys/types.h>
#endif

#ifdef	SVR4
#include <sys/types.h>
#ifndef	_POSIX_SOURCE
#define	_POSIX_SOURCE
#include <limits.h>
#undef	_POSIX_SOURCE
#else	/* _POSIX_SOURCE */
#include <limits.h>
#endif	/* !_POSIX_SOURCE */
#define NGROUPS NGROUPS_MAX
#endif	/* SVR4 */

ifdef(`SUN4', `#define	ATT_AGE')
ifdef(`SUN4', `#define  ATT_COMMENT')
ifdef(`SUN', `#define	BSD_QUOTAS')
ifdef(`BSD', `#define	BSD_QUOTAS')
ifdef(`USG', `#define	ATT_AGE')
ifdef(`USG', `#define	ATT_COMMENT')

/*
 * This is the data structure returned by the getpw* functions.  The
 * names of the elements and the structure are taken from traditional
 * usage.
 */

struct passwd	{
	char	*pw_name ;	/* User login name */
	char	*pw_passwd ;	/* Encrypted passwd or dummy field */
	uid_t	pw_uid ;	/* User uid number */
	gid_t	pw_gid ;	/* User group id number */
#ifdef	BSD_QUOTAS
	/* Most BSD systems have quotas, most USG ones don't	*/
	int	pw_quota ;	/* The BSD magic doodah */
#endif
#ifdef	ATT_AGE
	/* Use ATT-style password aging	*/
	char	*pw_age ;	/* ATT radix-64 encoded data */
#endif
#ifdef	ATT_COMMENT
	/* Provide the unused comment field */
	char	*pw_comment;	/* Unused comment field */
#endif
	char	*pw_gecos ;	/* ASCII user name, other data */
	char	*pw_dir ;	/* User home directory */
	char	*pw_shell ;	/* User startup shell */
} ;

#ifdef	ATT_COMMENT
/* Provide the unused comment structure */
struct comment {
	char	*c_dept;
	char	*c_name;
	char	*c_acct;
	char	*c_bin;
};
#endif

#if	__STDC__

extern	struct	passwd	*getpwent( void ) ;
extern	struct	passwd	*getpwuid( uid_t user_uid ) ;
extern	struct	passwd	*getpwnam( char *name ) ;
int	setpwent( void );
int	endpwent( void );

#else

extern	struct	passwd	*getpwent();
extern	struct	passwd	*getpwuid();
extern	struct	passwd	*getpwnam();
int	setpwent();
int	endpwent();

#endif	/* of if __STDC__ */

#endif	/* of ifdef PWD_H */
