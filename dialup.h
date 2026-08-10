/*
 * Copyright 1989, 1990, 1991, John F. Haugh II
 * All rights reserved.
 *
 * Permission is granted to copy and create derivative works for any
 * non-commercial purpose, provided this copyright notice is preserved
 * in all copies of source code, or included in human readable form
 * and conspicuously displayed on all copies of object code or
 * distribution media.
 */

/*
 * Structure of the /etc/d_passwd file
 *
 *	The d_passwd file contains the names of login shells which require
 *	dialup passwords.  Each line contains the fully qualified path name
 *	for the shell, followed by an optional password.  Each field is
 *	separated by a ':'.
 *
 * Structure of the /etc/dialups file
 *
 *	The dialups file contains the names of ports which may be dialup
 *	lines.  Each line consists of the last component of the path
 *	name.  The leading "/dev/" string is removed.
 *
 *	@(#)dialup.h	3.2	09:06:55	28 May 1991
 */

#ifndef	_DIALUP_H
#define	_DIALUP_H

struct	dialup {
	char	*du_shell;
	char	*du_passwd;
};

#if !__STDC__
extern	void	setduent ();
extern	void	endduent ();
extern	struct	dialup	*fgetduent ();
extern	struct	dialup	*getduent ();
extern	struct	dialup	*getdushell ();
extern	int	putduent ();
extern	int	isadialup ();
#else
extern	void	setduent (void);
extern	void	endduent (void);
extern	struct	dialup	*fgetduent (FILE *);
extern	struct	dialup	*getduent (void);
extern	struct	dialup	*getdushell (char *);
extern	int	putduent (struct dialup *, FILE *);
extern	int	isadialup (char *);
#endif

#define	DIALPWD	"/etc/d_passwd"
#define	DIALUPS	"/etc/dialups"

#endif
