/* $Id: chkname.h,v 1.2 2000/08/26 18:27:17 marekm Exp $ */
#ifndef _CHKNAME_H_
#define _CHKNAME_H_

/*
 * check_user_name(), check_group_name() - check the new user/group
 * name for validity; return value: 1 - OK, 0 - bad name
 */

#include "defines.h"

extern int check_user_name(const char *);
extern int check_group_name(const char *name);

#endif
