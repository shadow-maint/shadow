/* $Id$ */
/* some useful defines */

#ifndef _DEFINES_H_
#define _DEFINES_H_

#include "config.h"

#if HAVE_STDBOOL_H
# include <stdbool.h>
#else
# if ! HAVE__BOOL
#  ifdef __cplusplus
typedef bool _Bool;
#  else
typedef unsigned char _Bool;
#  endif
# endif
# define bool _Bool
# define false (0)
# define true  (1)
# define __bool_true_false_are_defined 1
#endif

/* Take care of NLS matters.  */
#ifdef S_SPLINT_S
extern char *setlocale(int categories, const char *locale);
# define LC_ALL		(6)
extern char * bindtextdomain (const char * domainname, const char * dirname);
extern char * textdomain (const char * domainname);
# define _(Text) Text
# define ngettext(Msgid1, Msgid2, N) \
    ((N) == 1 ? (const char *) (Msgid1) : (const char *) (Msgid2))
#else
#ifdef HAVE_LOCALE_H
# include <locale.h>
#else
# undef setlocale
# define setlocale(category, locale)	(NULL)
# ifndef LC_ALL
#  define LC_ALL	6
# endif
#endif

#define gettext_noop(String) (String)
/* #define gettext_def(String) "#define String" */

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(Text) gettext (Text)
#else
# undef bindtextdomain
# define bindtextdomain(Domain, Directory)	(NULL)
# undef textdomain
# define textdomain(Domain)	(NULL)
# define _(Text) Text
# define ngettext(Msgid1, Msgid2, N) \
    ((N) == 1 ? (const char *) (Msgid1) : (const char *) (Msgid2))
#endif
#endif

#include <stdlib.h>
#include <string.h>

#if HAVE_ERRNO_H
# include <errno.h>
#endif

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#if HAVE_UNISTD_H
# include <unistd.h>
#endif

/*
 * crypt(3), crypt_gensalt(3), and their
 * feature test macros may be defined in here.
 */
#if HAVE_CRYPT_H
# include <crypt.h>
#endif

#include <sys/time.h>
#include <time.h>

#ifdef HAVE_MEMSET_S
# define memzero(ptr, size) memset_s((ptr), 0, (size))
#elif defined HAVE_EXPLICIT_BZERO	/* !HAVE_MEMSET_S */
# define memzero(ptr, size) explicit_bzero((ptr), (size))
#else					/* !HAVE_MEMSET_S && HAVE_EXPLICIT_BZERO */
static inline void memzero(void *ptr, size_t size)
{
	volatile unsigned char * volatile p = ptr;
	while (size--) {
		*p++ = '\0';
	}
}
#endif					/* !HAVE_MEMSET_S && !HAVE_EXPLICIT_BZERO */

#define strzero(s) memzero(s, strlen(s))	/* warning: evaluates twice */

#include <dirent.h>

/*
 * Possible cases:
 * - /usr/include/shadow.h exists and includes the shadow group stuff.
 * - /usr/include/shadow.h exists, but we use our own gshadow.h.
 */
#include <shadow.h>
#if defined(SHADOWGRP) && !defined(GSHADOW)
#include "gshadow_.h"
#endif

#include <limits.h>

#ifndef	NGROUPS_MAX
#ifdef	NGROUPS
#define	NGROUPS_MAX	NGROUPS
#else
#define	NGROUPS_MAX	64
#endif
#endif

#ifdef USE_SYSLOG
#include <syslog.h>

#ifndef LOG_WARN
#define LOG_WARN LOG_WARNING
#endif

/* LOG_NOWAIT is deprecated */
#ifndef LOG_NOWAIT
#define LOG_NOWAIT 0
#endif

/* LOG_AUTH is deprecated, use LOG_AUTHPRIV instead */
#ifndef LOG_AUTHPRIV
#define LOG_AUTHPRIV LOG_AUTH
#endif

/* cleaner than lots of #ifdefs everywhere - use this as follows:
   SYSLOG((LOG_CRIT, "user %s cracked root", user)); */
#ifdef ENABLE_NLS
/* Temporarily set LC_TIME to "C" to avoid strange dates in syslog.
   This is a workaround for a more general syslog(d) design problem -
   syslogd should log the current system time for each event, and not
   trust the formatted time received from the unix domain (or worse,
   UDP) socket.  -MM */
/* Avoid translated PAM error messages: Set LC_ALL to "C".
 * --Nekral */
#define SYSLOG(x)							\
	do {								\
		char *old_locale = setlocale (LC_ALL, NULL);		\
		char *saved_locale = NULL;				\
		if (NULL != old_locale) {				\
			saved_locale = strdup (old_locale);		\
		}							\
		if (NULL != saved_locale) {				\
			(void) setlocale (LC_ALL, "C");			\
		}							\
		syslog x ;						\
		if (NULL != saved_locale) {				\
			(void) setlocale (LC_ALL, saved_locale);	\
			free (saved_locale);				\
		}							\
	} while (false)
#else				/* !ENABLE_NLS */
#define SYSLOG(x) syslog x
#endif				/* !ENABLE_NLS */

#else				/* !USE_SYSLOG */

#define SYSLOG(x)		/* empty */
#define openlog(a,b,c)		/* empty */
#define closelog()		/* empty */

#endif				/* !USE_SYSLOG */

/* The default syslog settings can now be changed here,
   in just one place.  */

#ifndef SYSLOG_OPTIONS
/* #define SYSLOG_OPTIONS (LOG_PID | LOG_CONS | LOG_NOWAIT) */
#define SYSLOG_OPTIONS (LOG_PID)
#endif

#ifndef SYSLOG_FACILITY
#define SYSLOG_FACILITY LOG_AUTHPRIV
#endif

#define OPENLOG(progname) openlog(progname, SYSLOG_OPTIONS, SYSLOG_FACILITY)

#ifndef F_OK
# define F_OK 0
# define X_OK 1
# define W_OK 2
# define R_OK 4
#endif

#ifndef SEEK_SET
# define SEEK_SET 0
# define SEEK_CUR 1
# define SEEK_END 2
#endif

#if HAVE_TERMIOS_H
# include <termios.h>
# define STTY(fd, termio) tcsetattr(fd, TCSANOW, termio)
# define GTTY(fd, termio) tcgetattr(fd, termio)
# define TERMIO struct termios
# define USE_TERMIOS
#else				/* assumed HAVE_TERMIO_H */
# include <sys/ioctl.h>
# include <termio.h>
# define STTY(fd, termio) ioctl(fd, TCSETA, termio)
# define GTTY(fd, termio) ioctl(fd, TCGETA, termio)
# define TEMRIO struct termio
# define USE_TERMIO
#endif

/*
 * Password aging constants
 *
 * DAY - seconds / day
 * WEEK - seconds / week
 * SCALE - seconds / aging unit
 */

/* Solaris defines this in shadow.h */
#ifndef DAY
#define DAY (24L*3600L)
#endif

#define WEEK (7*DAY)

#ifdef ITI_AGING
#define SCALE 1
#else
#define SCALE DAY
#endif

#define NITEMS(arr)  (sizeof((arr)) / sizeof((arr)[0]))

/* Copy string pointed by B to array A with size checking.  It was originally
   in lmain.c but is _very_ useful elsewhere.  Some setuid root programs with
   very sloppy coding used to assume that BUFSIZ will always be enough...  */

					/* danger - side effects */
#define STRFCPY(A,B) \
	(strncpy((A), (B), sizeof(A) - 1), (A)[sizeof(A) - 1] = '\0')

#ifndef PASSWD_FILE
#define PASSWD_FILE "/etc/passwd"
#endif

#ifndef GROUP_FILE
#define GROUP_FILE "/etc/group"
#endif

#ifndef SHADOW_FILE
#define SHADOW_FILE "/etc/shadow"
#endif

#ifdef SHADOWGRP
#ifndef SGROUP_FILE
#define SGROUP_FILE "/etc/gshadow"
#endif
#endif

#ifndef NULL
#define NULL ((void *) 0)
#endif

#ifdef sun			/* hacks for compiling on SunOS */
# ifndef SOLARIS
extern int fputs ();
extern char *strdup ();
extern char *strerror ();
# endif
#endif

/*
 * string to use for the pw_passwd field in /etc/passwd when using
 * shadow passwords - most systems use "x" but there are a few
 * exceptions, so it can be changed here if necessary.  --marekm
 */
#ifndef SHADOW_PASSWD_STRING
#define SHADOW_PASSWD_STRING "x"
#endif

#define SHADOW_SP_FLAG_UNSET ((unsigned long int)-1)

#ifdef WITH_AUDIT
#ifdef __u8			/* in case we use pam < 0.80 */
#undef __u8
#endif
#ifdef __u32
#undef __u32
#endif

#include <libaudit.h>
#endif

/* To be used for verified unused parameters */
#if defined(__GNUC__) && !defined(__STRICT_ANSI__)
# define unused __attribute__((unused))
# define format_attr(type, index, check) __attribute__((format (type, index, check)))
#else
# define unused
# define format_attr(type, index, check)
#endif

/* Maximum length of usernames */
#ifdef HAVE_UTMPX_H
# include <utmpx.h>
# define USER_NAME_MAX_LENGTH (sizeof (((struct utmpx *)NULL)->ut_user))
#else
# include <utmp.h>
# ifdef HAVE_STRUCT_UTMP_UT_USER
#  define USER_NAME_MAX_LENGTH (sizeof (((struct utmp *)NULL)->ut_user))
# else
#  ifdef HAVE_STRUCT_UTMP_UT_NAME
#   define USER_NAME_MAX_LENGTH (sizeof (((struct utmp *)NULL)->ut_name))
#  else
#   define USER_NAME_MAX_LENGTH 32
#  endif
# endif
#endif

/* Maximum length of passwd entry */
#define PASSWD_ENTRY_MAX_LENGTH 32768

#ifdef HAVE_SECURE_GETENV
#  define shadow_getenv(name) secure_getenv(name)
# else
#  define shadow_getenv(name) getenv(name)
#endif

#endif				/* _DEFINES_H_ */
