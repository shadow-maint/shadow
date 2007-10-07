/* $XFree86: xc/lib/misc/snprintf.h,v 3.1 1996/08/26 14:42:33 dawes Exp $ */

#ifndef SNPRINTF_H
#define SNPRINTF_H

#ifdef HAS_SNPRINTF
#ifdef LIBXT
#define _XtSnprintf snprintf
#define _XtVsnprintf vsnprintf
#endif
#ifdef LIBX11
#define _XSnprintf snprintf
#define _XVsnprintf vsnprintf
#endif
#else /* !HAS_SNPRINTF */

#ifdef LIBXT
#define snprintf _XtSnprintf
#define vsnprintf _XtVsnprintf
#endif
#ifdef LIBX11
#define snprintf _XSnprintf
#define vsnprintf _XVsnprintf
#endif

#if 1  /* the system might have no X11 headers.  -MM */
#include <X11/Xos.h>
#include <X11/Xlib.h>
#else  /* but we still need this... */
#include <sys/types.h>
/* adjust the following defines if necessary (pre-ANSI) */
#define NeedFunctionPrototypes 1
#define NeedVarargsPrototypes 1
#endif

#if NeedVarargsPrototypes
#define HAVE_STDARG_H
#endif

#ifdef HAVE_STDARG_H
#include <stdarg.h>
extern int snprintf (char *str, size_t count, const char *fmt, ...);
extern int vsnprintf (char *str, size_t count, const char *fmt, va_list arg);
#else
extern int snprintf ();
extern int vsnprintf ();
#endif

#endif /* HAS_SNPRINTF */

#endif /* SNPRINTF_H */
