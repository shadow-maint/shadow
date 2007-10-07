/*
 * Copyright 1989 - 1991, Julianne Frances Haugh
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Julianne F. Haugh nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY JULIE HAUGH AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL JULIE HAUGH OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
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
 *	$Id: dialup.h,v 1.3 2000/08/26 18:27:17 marekm Exp $
 */

#ifndef	_DIALUP_H
#define	_DIALUP_H

struct	dialup {
	char	*du_shell;
	char	*du_passwd;
};

extern void setduent(void);
extern void endduent(void);
extern struct dialup *fgetduent(FILE *);
extern struct dialup *getduent(void);
extern struct dialup *getdushell(const char *);
extern int putduent(const struct dialup *, FILE *);
extern int isadialup(const char *);

#define	DIALPWD	"/etc/d_passwd"
#define	DIALUPS	"/etc/dialups"

#endif
