/* $Id$ */
#ifndef _FAILURE_H_
#define _FAILURE_H_

#include "defines.h"
#include "faillog.h"
#ifdef HAVE_UTMPX_H
#include <utmpx.h>
#else
#include <utmp.h>
#endif

/*
 * failure - make failure entry
 *
 *	failure() creates a new (struct faillog) entry or updates an
 *	existing one with the current failed login information.
 */
extern void failure (uid_t, const char *, struct faillog *);

/*
 * failcheck - check for failures > allowable
 *
 *	failcheck() is called AFTER the password has been validated.  If the
 *	account has been "attacked" with too many login failures, failcheck()
 *	returns FALSE to indicate that the login should be denied even though
 *	the password is valid.
 */
extern int failcheck (uid_t, struct faillog *, int);

/*
 * failprint - print line of failure information
 *
 *	failprint takes a (struct faillog) entry and formats it into a
 *	message which is displayed at login time.
 */
extern void failprint (const struct faillog *);

/*
 * failtmp - update the cummulative failure log
 *
 *	failtmp updates the (struct utmp) formatted failure log which
 *	maintains a record of all login failures.
 */
#ifdef HAVE_UTMPX_H
extern void failtmp (const struct utmpx *);
#else
extern void failtmp (const struct utmp *);
#endif

#endif
