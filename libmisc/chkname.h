/* $Id: chkname.h,v 1.1 1997/12/07 23:27:00 marekm Exp $ */
#ifndef _CHKNAME_H_
#define _CHKNAME_H_

/*
 * check_user_name(), check_group_name() - check the new user/group
 * name for validity; return value: 1 - OK, 0 - bad name
 */

#include "defines.h"

extern int check_user_name P_((const char *));
extern int check_group_name P_((const char *name));

#endif
