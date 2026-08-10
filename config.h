/*
 * Copyright 1989, 1990, 1991, 1992, 1993, John F. Haugh II
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
 * Configuration file for login.
 *
 *	@(#)config.h	3.16.1.3	17:52:38	09 May 1993
 */


/*
 * Pathname to the run-time configuration definitions file.
 */

#define LOGINDEFS "/etc/login.defs"

/*
 * Define SHADOWPWD to use shadow [ unreadable ] password file.
 * Release 3 has a requirement that SHADOWPWD always be defined.
 */

#define	SHADOWPWD

/*
 * Define AUTOSHADOW to have root always copy sp_pwdp to pw_passwd
 * for getpwuid() and getpwnam().  This provides compatibility for
 * privileged applications which are shadow-ignorant.  YOU ARE
 * ENCOURAGED TO NOT USE THIS OPTION UNLESS ABSOLUTELY NECESSARY.
 */

#undef	AUTOSHADOW

/*
 * Define SHADOWGRP to user shadowed group files.  This feature adds
 * the concept of a group administrator.  You MUST NOT define this
 * if you disable SHADOWPWD.
 */

/* #define	SHADOWGRP /**/

/*
 * Define DOUBLESIZE to use 16 character passwords
 */

#define DOUBLESIZE

/*
 * Define AGING if you want the password aging checks made.
 * Release 3 has a requirement that AGING always be defined.
 */

#define	AGING

/*
 * Pick your version of DBM.  If you define either DBM or NDBM, you must
 * define GETPWENT.  If you define NDBM you must define GETGRENT as well.
 */

/* #define	DBM	/**/
/* #define	NDBM	/**/

/*
 * Define USE_SYSLOG if you want to have SYSLOG functions included in your code.
 */

#define	USE_SYSLOG

/*
 * Enable RLOGIN to support the "-r" and "-h" options.
 * Also enable UT_HOST if your /etc/utmp provides for a host name.
 */

#define RLOGIN
#undef UT_HOST

/*
 * Define the "success" code from ruserok().  Most modern systems use 0
 * for success and -1 for failure, while certain older versions use 1
 * for success and 0 for failure.  Please check your manpage to be sure.
 */

#define	RUSEROK	0

/*
 * Select one of the following
 */

#define DIR_XENIX	/* include <sys/ndir.h>, use (struct direct)	*/
/* #define DIR_BSD	/* include <ndir.h>, use (struct direct)	*/
/* #define DIR_SYSV	/* include <dirent.h>, use (struct dirent)	*/

/*
 * Various system environment definitions.
 */

#define	HAVE_ULIMIT	/* Define if your UNIX supports ulimit()	*/
#undef	HAVE_RLIMIT	/* Define if your UNIX supports setrlimit()     */
#define	GETPWENT	/* Define if you want my GETPWENT(3) routines	*/
#define	GETGRENT	/* Define if you want my GETGRENT(3) routines	*/
#define	NEED_AL64	/* Define if library does not include a64l()	*/
#define	NEED_MKDIR	/* Define if system does not have mkdir()	*/
#define	NEED_RMDIR	/* Define if system does not have rmdir()	*/
#define	NEED_RENAME	/* Define if system does not have rename()	*/
#define	NEED_STRSTR	/* Define if library does not include strstr()	*/
#undef	NEED_PUTPWENT	/* Define if library does not include putpwent()*/
#define	SIGTYPE	int	/* Type returned by signal()                    */

/*
 * These definitions MUST agree with the values defined in <pwd.h>.
 */

#undef	BSD_QUOTA	/* the pw_quota field exists */
#define	ATT_AGE		/* the pw_age field exists */
#define	ATT_COMMENT	/* the pw_comment field exists */

#define	UID_T	uid_t	/* set to be the type of UID's */
#define	GID_T	gid_t	/* set to be the type of GID's */

#ifndef	UID_T
#if defined(SVR4) || defined(_POSIX_SOURCE)
#define	UID_T	uid_t
#else
#define	UID_T	int
#endif
#endif

#ifndef	GID_T
#if defined(SVR4) || defined(_POSIX_SOURCE)
#define	GID_T	gid_t
#else
#define	GID_T	int
#endif
#endif

/*
 * Define NDEBUG for production versions
 */

#define	NDEBUG

/*
 * Define PWDFILE and GRPFILE to the names of the password and
 * group files.
 */

#define	PWDFILE	"/etc/passwd"
#define	GRPFILE	"/etc/group"

/*
 * The structure of the utmp file.  There are two kinds of UTMP files,
 * "BSD" and "USG".  "BSD" has no PID or type information, "USG" does.
 * If you define neither of these, the type will be defaulted by using
 * BSD, SUN, SYS3 and USG defines.
 */

#define USG_UTMP	/**/
/* #define BSD_UTMP	/**/

#if !defined(USG_UTMP) && !defined(BSD_UTMP)
#if defined(BSD) || defined(SYS3) || defined(SUN)
#define	BSD_UTMP
#else
#define USG_UTMP
#endif	/* BSD || SYS3 || SUN */
#endif /* !USG_UTMP || !BSD_UTMP */

/*
 * Telinit program.  If your system uses /etc/telinit to change run
 * level, define TELINIT and then define the RUNLEVEL macro to be the
 * run-level to switch INIT to.  This is used by sulogin to change
 * from single user to multi-user mode.
 */

#define	TELINIT		/**/
#define	RUNLEVEL	"2"	/**/

/*
 * Crontab and atrm.  If your system can "crontab -r -u <user>", define
 * HAS_CRONTAB.  If your system can "atrm <user>", define HAS_ATRM.
 */

#undef	HAS_CRONTAB
#undef	HAS_ATRM
