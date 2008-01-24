/*
 * Copyright 1989 - 1994, Julianne Frances Haugh
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
 * Separated from setup.c.  --marekm
 * Resource limits thanks to Cristian Gafton.
 */

#include <config.h>

#ifndef USE_PAM

#ident "$Id$"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include "prototypes.h"
#include "defines.h"
#include <pwd.h>
#include "getdef.h"
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#define LIMITS
#endif
#ifdef LIMITS
#ifndef LIMITS_FILE
#define LIMITS_FILE "/etc/limits"
#endif
#define LOGIN_ERROR_RLIMIT	1
#define LOGIN_ERROR_LOGIN	2
/* Set a limit on a resource */
/*
 *	rlimit - RLIMIT_XXXX
 *	value - string value to be read
 *	multiplier - value*multiplier is the actual limit
 */
static int
setrlimit_value (unsigned int rlimit, const char *value,
		 unsigned int multiplier)
{
	struct rlimit rlim;
	long limit;
	char **endptr = (char **) &value;
	const char *value_orig = value;

	limit = strtol (value, endptr, 10);
	if (limit == 0 && value_orig == *endptr)	/* no chars read */
		return 0;
	limit *= multiplier;
	rlim.rlim_cur = limit;
	rlim.rlim_max = limit;
	if (setrlimit (rlimit, &rlim))
		return LOGIN_ERROR_RLIMIT;
	return 0;
}


static int set_prio (const char *value)
{
	int prio;
	char **endptr = (char **) &value;

	prio = strtol (value, endptr, 10);
	if ((prio == 0) && (value == *endptr))
		return 0;
	if (setpriority (PRIO_PROCESS, 0, prio))
		return LOGIN_ERROR_RLIMIT;
	return 0;
}


static int set_umask (const char *value)
{
	mode_t mask;
	char **endptr = (char **) &value;

	mask = strtol (value, endptr, 8) & 0777;
	if ((mask == 0) && (value == *endptr))
		return 0;
	umask (mask);
	return 0;
}


/* Counts the number of user logins and check against the limit */
static int check_logins (const char *name, const char *maxlogins)
{
#if HAVE_UTMPX_H
	struct utmpx *ut;
#else
	struct utmp *ut;
#endif
	unsigned int limit, count;
	char **endptr = (char **) &maxlogins;
	const char *ml_orig = maxlogins;

	limit = strtol (maxlogins, endptr, 10);
	if (limit == 0 && ml_orig == *endptr)	/* no chars read */
		return 0;

	if (limit == 0) {	/* maximum 0 logins ? */
		SYSLOG ((LOG_WARN, "No logins allowed for `%s'\n", name));
		return LOGIN_ERROR_LOGIN;
	}

	count = 0;
#if HAVE_UTMPX_H
	setutxent ();
	while ((ut = getutxent ())) {
#else
	setutent ();
	while ((ut = getutent ())) {
#endif
		if (ut->ut_type != USER_PROCESS)
			continue;
		if (ut->ut_user[0] == '\0')
			continue;
		if (strncmp (name, ut->ut_user, sizeof (ut->ut_user)) != 0)
			continue;
		if (++count > limit)
			break;
	}
#if HAVE_UTMPX_H
	endutxent ();
#else
	endutent ();
#endif
	/*
	 * This is called after setutmp(), so the number of logins counted
	 * includes the user who is currently trying to log in.
	 */
	if (count > limit) {
		SYSLOG ((LOG_WARN, "Too many logins (max %d) for %s\n",
			 limit, name));
		return LOGIN_ERROR_LOGIN;
	}
	return 0;
}

/* Function setup_user_limits - checks/set limits for the curent login
 * Original idea from Joel Katz's lshell. Ported to shadow-login
 * by Cristian Gafton - gafton@sorosis.ro
 *
 * We are passed a string of the form ('BASH' constants for ulimit)
 *     [Aa][Cc][Dd][Ff][Mm][Nn][Rr][Ss][Tt][Uu][Ll][Pp][Ii][Oo]
 *     (eg. 'C2F256D2048N5' or 'C2 F256 D2048 N5')
 * where:
 * [Aa]: a = RLIMIT_AS		max address space (KB)
 * [Cc]: c = RLIMIT_CORE	max core file size (KB)
 * [Dd]: d = RLIMIT_DATA	max data size (KB)
 * [Ff]: f = RLIMIT_FSIZE	max file size (KB)
 * [Mm]: m = RLIMIT_MEMLOCK	max locked-in-memory address space (KB)
 * [Nn]: n = RLIMIT_NOFILE	max number of open files
 * [Rr]: r = RLIMIT_RSS		max resident set size (KB)
 * [Ss]: s = RLIMIT_STACK	max stack size (KB)
 * [Tt]: t = RLIMIT_CPU		max CPU time (MIN)
 * [Uu]: u = RLIMIT_NPROC	max number of processes
 * [Kk]: k = file creation masK (umask)
 * [Ll]: l = max number of logins for this user
 * [Pp]: p = process priority -20..20 (negative = high, positive = low)
 * [Ii]: i = RLIMIT_NICE    max nice value (0..39 translates to 20..-19)
 * [Oo]: o = RLIMIT_RTPRIO  max real time priority (linux/sched.h 0..MAX_RT_PRIO)
 *
 * Return value:
 *		0 = okay, of course
 *		LOGIN_ERROR_RLIMIT = error setting some RLIMIT
 *		LOGIN_ERROR_LOGIN  = error - too many logins for this user
 *
 * buf - the limits string
 * name - the username
 */
static int do_user_limits (const char *buf, const char *name)
{
	const char *pp;
	int retval = 0;

	pp = buf;

	while (*pp != '\0')
		switch (*pp++) {
#ifdef RLIMIT_AS
		case 'a':
		case 'A':
			/* RLIMIT_AS - max address space (KB) */
			retval |= setrlimit_value (RLIMIT_AS, pp, 1024);
			break;
#endif
#ifdef RLIMIT_CPU
		case 't':
		case 'T':
			/* RLIMIT_CPU - max CPU time (MIN) */
			retval |= setrlimit_value (RLIMIT_CPU, pp, 60);
			break;
#endif
#ifdef RLIMIT_DATA
		case 'd':
		case 'D':
			/* RLIMIT_DATA - max data size (KB) */
			retval |= setrlimit_value (RLIMIT_DATA, pp, 1024);
			break;
#endif
#ifdef RLIMIT_FSIZE
		case 'f':
		case 'F':
			/* RLIMIT_FSIZE - Maximum filesize (KB) */
			retval |= setrlimit_value (RLIMIT_FSIZE, pp, 1024);
			break;
#endif
#ifdef RLIMIT_NPROC
		case 'u':
		case 'U':
			/* RLIMIT_NPROC - max number of processes */
			retval |= setrlimit_value (RLIMIT_NPROC, pp, 1);
			break;
#endif
#ifdef RLIMIT_CORE
		case 'c':
		case 'C':
			/* RLIMIT_CORE - max core file size (KB) */
			retval |= setrlimit_value (RLIMIT_CORE, pp, 1024);
			break;
#endif
#ifdef RLIMIT_MEMLOCK
		case 'm':
		case 'M':
			/* RLIMIT_MEMLOCK - max locked-in-memory address space (KB) */
			retval |= setrlimit_value (RLIMIT_MEMLOCK, pp, 1024);
			break;
#endif
#ifdef RLIMIT_NOFILE
		case 'n':
		case 'N':
			/* RLIMIT_NOFILE - max number of open files */
			retval |= setrlimit_value (RLIMIT_NOFILE, pp, 1);
			break;
#endif
#ifdef RLIMIT_RSS
		case 'r':
		case 'R':
			/* RLIMIT_RSS - max resident set size (KB) */
			retval |= setrlimit_value (RLIMIT_RSS, pp, 1024);
			break;
#endif
#ifdef RLIMIT_STACK
		case 's':
		case 'S':
			/* RLIMIT_STACK - max stack size (KB) */
			retval |= setrlimit_value (RLIMIT_STACK, pp, 1024);
			break;
#endif
#ifdef RLIMIT_NICE
		case 'i':
		case 'I':
			/* RLIMIT_NICE - max scheduling priority (0..39) */
			retval |= setrlimit_value (RLIMIT_NICE, pp, 1);
			break;
#endif
#ifdef RLIMIT_RTPRIO
		case 'o':
		case 'O':
			/* RLIMIT_RTPRIO - max real time priority (0..MAX_RT_PRIO) */
			retval |= setrlimit_value (RLIMIT_RTPRIO, pp, 1);
			break;
#endif
		case 'k':
		case 'K':
			retval |= set_umask (pp);
			break;
		case 'l':
		case 'L':
			/* LIMIT the number of concurent logins */
			retval |= check_logins (name, pp);
			break;
		case 'p':
		case 'P':
			retval |= set_prio (pp);
			break;
		}
	return retval;
}

static int setup_user_limits (const char *uname)
{
	/* TODO: allow and use @group syntax --cristiang */
	FILE *fil;
	char buf[1024];
	char name[1024];
	char limits[1024];
	char deflimits[1024];
	char tempbuf[1024];

	/* init things */
	memzero (buf, sizeof (buf));
	memzero (name, sizeof (name));
	memzero (limits, sizeof (limits));
	memzero (deflimits, sizeof (deflimits));
	memzero (tempbuf, sizeof (tempbuf));

	/* start the checks */
	fil = fopen (LIMITS_FILE, "r");
	if (fil == NULL) {
		return 0;
	}
	/* The limits file have the following format:
	 * - '#' (comment) chars only as first chars on a line;
	 * - username must start on first column
	 * A better (smarter) checking should be done --cristiang */
	while (fgets (buf, 1024, fil) != NULL) {
		if (buf[0] == '#' || buf[0] == '\n')
			continue;
		memzero (tempbuf, sizeof (tempbuf));
		/* a valid line should have a username, then spaces,
		 * then limits
		 * we allow the format:
		 * username    L2  D2048  R4096
		 * where spaces={' ',\t}. Also, we reject invalid limits.
		 * Imposing a limit should be done with care, so a wrong
		 * entry means no care anyway :-). A '-' as a limits
		 * strings means no limits --cristiang */
		if (sscanf (buf, "%s%[ACDFMNRSTULPIOacdfmnrstulpio0-9 \t-]",
			    name, tempbuf) == 2) {
			if (strcmp (name, uname) == 0) {
				strcpy (limits, tempbuf);
				break;
			} else if (strcmp (name, "*") == 0) {
				strcpy (deflimits, tempbuf);
			}
		}
	}
	fclose (fil);
	if (limits[0] == '\0') {
		/* no user specific limits */
		if (deflimits[0] == '\0')	/* no default limits */
			return 0;
		strcpy (limits, deflimits);	/* use the default limits */
	}
	return do_user_limits (limits, uname);
}
#endif				/* LIMITS */


static void setup_usergroups (const struct passwd *info)
{
	const struct group *grp;
	mode_t oldmask;

/*
 *	if not root, and UID == GID, and username is the same as primary
 *	group name, set umask group bits to be the same as owner bits
 *	(examples: 022 -> 002, 077 -> 007).
 */
	if (info->pw_uid != 0 && info->pw_uid == info->pw_gid) {
		/* local, no need for xgetgrgid */
		grp = getgrgid (info->pw_gid);
		if (grp && (strcmp (info->pw_name, grp->gr_name) == 0)) {
			oldmask = umask (0777);
			umask ((oldmask & ~070) | ((oldmask >> 3) & 070));
		}
	}
}

/*
 *	set the process nice, ulimit, and umask from the password file entry
 */

void setup_limits (const struct passwd *info)
{
	char *cp;
	int i;
	long l;

	if (getdef_bool ("USERGROUPS_ENAB"))
		setup_usergroups (info);

	/*
	 * See if the GECOS field contains values for NICE, UMASK or ULIMIT.
	 * If this feature is enabled in /etc/login.defs, we make those
	 * values the defaults for this login session.
	 */

	if (getdef_bool ("QUOTAS_ENAB")) {
#ifdef LIMITS
		if (info->pw_uid != 0)
			if (setup_user_limits (info->pw_name) &
			    LOGIN_ERROR_LOGIN) {
				fputs (_("Too many logins.\n"), stderr);
				sleep (2);
				exit (1);
			}
#endif
		for (cp = info->pw_gecos; cp != NULL; cp = strchr (cp, ',')) {
			if (*cp == ',')
				cp++;

			if (strncmp (cp, "pri=", 4) == 0) {
				i = atoi (cp + 4);
				if (i >= -20 && i <= 20)
					(void) nice (i);

				continue;
			}
			if (strncmp (cp, "ulimit=", 7) == 0) {
				l = strtol (cp + 7, (char **) 0, 10);
				set_filesize_limit (l);
				continue;
			}
			if (strncmp (cp, "umask=", 6) == 0) {
				i = strtol (cp + 6, (char **) 0, 8) & 0777;
				(void) umask (i);

				continue;
			}
		}
	}
}

#else				/* !USE_PAM */
extern int errno;		/* warning: ANSI C forbids an empty source file */
#endif				/* !USE_PAM */
