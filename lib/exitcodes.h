/* $Id: exitcodes.h,v 1.7 2005/08/31 17:36:11 kloczek Exp $ */

/*
 * Exit codes used by shadow programs
 */
#define E_SUCCESS       	0	/* success */
#define E_NOPERM        	1	/* permission denied */
#define E_USAGE         	2	/* invalid command syntax */
#define E_BAD_ARG       	3	/* invalid argument to option */
#define E_PASSWD_NOTFOUND	14	/* not found password file */
#define E_SHADOW_NOTFOUND	15	/* not found shadow password file */
#define E_GROUP_NOTFOUND	16	/* not found group file */
#define E_GSHADOW_NOTFOUND	17	/* not found shadow group file */
