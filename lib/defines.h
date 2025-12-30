/* $Id$ */
/* some useful defines */

#ifndef _DEFINES_H_
#define _DEFINES_H_

#include "config.h"

#include <stdbool.h>
#include <locale.h>

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <unistd.h>

/*
 * crypt(3), crypt_gensalt(3), and their
 * feature test macros may be defined in here.
 */
#if __has_include(<crypt.h>)
# include <crypt.h>
#endif

#include <sys/time.h>
#include <time.h>

#include <dirent.h>

#include <shadow.h>

#include <limits.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64
#endif

#include "io/syslog.h"

#include <termios.h>
#define STTY(fd, termio) tcsetattr(fd, TCSANOW, termio)
#define GTTY(fd, termio) tcgetattr(fd, termio)
#define TERMIO struct termios

/*
 * Password aging constants
 *
 * DAY - seconds / day
 * WEEK - seconds / week
 */

/* Solaris defines this in shadow.h */
#ifndef DAY
#define DAY  ((time_t) 24 * 3600)
#endif

#define WEEK (7*DAY)

#ifndef PASSWD_FILE
#define PASSWD_FILE "/etc/passwd"
#endif

#ifndef GROUP_FILE
#define GROUP_FILE "/etc/group"
#endif

#ifndef SUBUID_FILE
#define SUBUID_FILE "/etc/subuid"
#endif

#ifndef SUBGID_FILE
#define SUBGID_FILE "/etc/subgid"
#endif

/*
 * string to use for the pw_passwd field in /etc/passwd when using
 * shadow passwords - most systems use "x" but there are a few
 * exceptions, so it can be changed here if necessary.  --marekm
 */
#ifndef SHADOW_PASSWD_STRING
#define SHADOW_PASSWD_STRING "x"
#endif

#define SHADOW_SP_FLAG_UNSET ((unsigned long)-1)

#ifdef WITH_AUDIT
/* in case we use pam < 0.80 */
#undef __u8
#undef __u32

#include <libaudit.h>
#endif

/* Maximum length of passwd entry */
#define PASSWD_ENTRY_MAX_LENGTH 32768

#ifdef HAVE_SECURE_GETENV
#  define shadow_getenv(name) secure_getenv(name)
# else
#  define shadow_getenv(name) getenv(name)
#endif

/*
 * Maximum password length
 *
 * Consider that there is also limit in PAM (PAM_MAX_RESP_SIZE)
 * currently set to 512.
 */
#if !defined(PASS_MAX)
#define PASS_MAX  BUFSIZ - 1
#endif

#endif				/* _DEFINES_H_ */
