/*
 * Copyright 1990, 1991, 1992, Julianne Frances Haugh and Steve Simmons
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

/*
 * Standard definitions for password files.  This is an independant
 * reimplementation of the definitions used by AT&T, BSD, and POSIX.
 * It is not derived from any of those sources.  Note that it can be
 * site-defined to have non-POSIX features as well.  Ideally this file
 * is simply replaced by the standard system supplied /usr/include/pwd.h
 * file.
 *
 *	@(#)pwd.h.m4	3.4.1.3	12:55:53	05 Feb 1994
 *	$Id: pwd.h.m4,v 1.2 1997/05/01 23:11:59 marekm Exp $
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
ifdef(`SUN', `#define	BSD_QUOTA')
ifdef(`BSD', `#define	BSD_QUOTA')
ifdef(`AIX', `', `ifdef(`USG', `#define	ATT_AGE')')
ifdef(`AIX', `', `ifdef(`USG', `#define	ATT_COMMENT')')

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
#ifdef	BSD_QUOTA
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
#ifdef	SVR4
void	setpwent( void );
void	endpwent( void );
#else
int	setpwent( void );
int	endpwent( void );
#endif

#else

extern	struct	passwd	*getpwent();
extern	struct	passwd	*getpwuid();
extern	struct	passwd	*getpwnam();
#ifdef	SVR4
void	setpwent();
void	endpwent();
#else
int	setpwent();
int	endpwent();
#endif
#endif	/* of if __STDC__ */

#endif	/* of ifdef PWD_H */
