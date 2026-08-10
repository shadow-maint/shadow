/*
 * Copyright 1989, 1990, 1992, John F. Haugh II
 * All rights reserved.
 *
 * Permission is granted to copy and create derivative works for any
 * non-commercial purpose, provided this copyright notice is preserved
 * in all copies of source code, or included in human readable form
 * and conspicuously displayed on all copies of object code or
 * distribution media.
 */

/*
 * faillog.h - login failure logging file format
 *
 *	@(#)faillog.h	3.1	20:36:28	07 Mar 1992
 *
 * The login failure file is maintained by login(1) and faillog(8)
 * Each record in the file represents a separate UID and the file
 * is indexed in that fashion.
 */

#ifdef	SVR4
#define	FAILFILE	"/var/adm/faillog"
#else
#define	FAILFILE	"/usr/adm/faillog"
#endif

struct	faillog {
	short	fail_cnt;	/* failures since last success */
	short	fail_max;	/* failures before turning account off */
	char	fail_line[12];	/* last failure occured here */
	time_t	fail_time;	/* last failure occured then */
};
