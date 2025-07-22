// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_IO_SYSLOG_H_
#define SHADOW_INCLUDE_LIB_IO_SYSLOG_H_


#include "config.h"

#include <locale.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>


#ifndef LOG_WARN
#define LOG_WARN LOG_WARNING
#endif

/* LOG_AUTH is deprecated, use LOG_AUTHPRIV instead */
#ifndef LOG_AUTHPRIV
#define LOG_AUTHPRIV LOG_AUTH
#endif

/* cleaner than lots of #ifdefs everywhere - use this as follows:
   SYSLOG(LOG_CRIT, "user %s cracked root", user); */
#ifdef ENABLE_NLS
/* Temporarily set LC_TIME to "C" to avoid strange dates in syslog.
   This is a workaround for a more general syslog(d) design problem -
   syslogd should log the current system time for each event, and not
   trust the formatted time received from the unix domain (or worse,
   UDP) socket.  -MM */
/* Avoid translated PAM error messages: set LC_ALL to "C".
 * --Nekral */
#define SYSLOG(...)							\
	do {								\
		char *old_locale = setlocale (LC_ALL, NULL);		\
		char *saved_locale = NULL;				\
		if (NULL != old_locale) {				\
			saved_locale = strdup (old_locale);		\
		}							\
		if (NULL != saved_locale) {				\
			(void) setlocale (LC_ALL, "C");			\
		}							\
		syslog(__VA_ARGS__);					\
		if (NULL != saved_locale) {				\
			(void) setlocale (LC_ALL, saved_locale);	\
			free (saved_locale);				\
		}							\
	} while (false)
#else				/* !ENABLE_NLS */
#define SYSLOG(...)  syslog(__VA_ARGS__)
#endif				/* !ENABLE_NLS */

/* The default syslog settings can now be changed here,
   in just one place.  */

#ifndef SYSLOG_OPTIONS
/* #define SYSLOG_OPTIONS (LOG_PID | LOG_CONS) */
#define SYSLOG_OPTIONS (LOG_PID)
#endif

#ifndef SYSLOG_FACILITY
#define SYSLOG_FACILITY LOG_AUTHPRIV
#endif

#define OPENLOG(progname) openlog(progname, SYSLOG_OPTIONS, SYSLOG_FACILITY)


#endif  // include guard
