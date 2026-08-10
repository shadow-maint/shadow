/*
 * Copyright 1988, 1989, 1990, John F. Haugh II
 * All rights reserved.
 *
 * Use, duplication, and disclosure prohibited without
 * the express written permission of the author.
 */

#ifndef	_H_SHADOW
#define	_H_SHADOW

/*
 * This information is not derived from AT&T licensed sources.  Posted
 * to the USENET 11/88, and updated 11/90 with information from SVR4.
 *
 *	@(#)shadow.h	3.3	09:06:50	07 Dec 1990
 */

#ifdef	ITI_AGING
typedef	time_t	sptime;
#else
typedef	long	sptime;
#endif

/*
 * Shadow password security file structure.
 */

struct	spwd {
	char	*sp_namp;	/* login name */
	char	*sp_pwdp;	/* encrypted password */
	sptime	sp_lstchg;	/* date of last change */
	sptime	sp_min;		/* minimum number of days between changes */
	sptime	sp_max;		/* maximum number of days between changes */
	sptime	sp_warn;	/* number of days of warning before password
				   expires */
	sptime	sp_inact;	/* number of days after password expires
				   until the account becomes unusable. */
	sptime	sp_expire;	/* days since 1/1/70 until account expires */
	unsigned long	sp_flag; /* reserved for future use */
};

/*
 * Shadow password security file functions.
 */

struct	spwd	*getspent ();
struct	spwd	*getspnam ();
struct	spwd	*sgetspent ();
struct	spwd	*fgetspent ();
void	setspent ();
void	endspent ();
int	putspent ();

#define  SHADOW "/etc/shadow"

/*
 * Shadow group security file structure
 */

struct	sgrp {
	char	*sg_name;	/* group name */
	char	*sg_passwd;	/* group password */
	char	**sg_adm;	/* group administator list */
	char	**sg_mem;	/* group membership list */
};

/*
 * Shadow group security file functions.
 */

struct	sgrp	*getsgent ();
struct	sgrp	*getsgnam ();
struct	sgrp	*sgetsgent ();
struct	sgrp	*fgetsgent ();
void	setsgent ();
void	endsgent ();
int	putsgent ();

#define	GSHADOW	"/etc/gshadow"
#endif
