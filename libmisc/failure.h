/* $Id: failure.h,v 1.1 1997/12/07 23:27:04 marekm Exp $ */
#ifndef _FAILURE_H_
#define _FAILURE_H_

#include "defines.h"
#include "faillog.h"
#include <utmp.h>

/*
 * failure - make failure entry
 *
 *	failure() creates a new (struct faillog) entry or updates an
 *	existing one with the current failed login information.
 */
extern void failure P_((uid_t, const char *, struct faillog *));

/*
 * failcheck - check for failures > allowable
 *
 *	failcheck() is called AFTER the password has been validated.  If the
 *	account has been "attacked" with too many login failures, failcheck()
 *	returns FALSE to indicate that the login should be denied even though
 *	the password is valid.
 */
extern int failcheck P_((uid_t, struct faillog *, int));

/*
 * failprint - print line of failure information
 *
 *	failprint takes a (struct faillog) entry and formats it into a
 *	message which is displayed at login time.
 */
extern void failprint P_((const struct faillog *));

/*
 * failtmp - update the cummulative failure log
 *
 *	failtmp updates the (struct utmp) formatted failure log which
 *	maintains a record of all login failures.
 */
extern void failtmp P_((const struct utmp *));

#endif

