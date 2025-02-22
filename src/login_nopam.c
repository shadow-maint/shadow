/* Taken from logdaemon-5.0, only minimal changes.  --marekm */
/*
 * SPDX-FileCopyrightText: 1990 - 1995, Wietse Venema.
 *
 * SPDX-License-Identifier: Unlicense
 */

/************************************************************************
* Copyright 1995 by Wietse Venema.  All rights reserved. Individual files
* may be covered by other copyrights (as noted in the file itself.)
*
* This material was originally written and compiled by Wietse Venema at
* Eindhoven University of Technology, The Netherlands, in 1990, 1991,
* 1992, 1993, 1994 and 1995.
*
* Redistribution and use in source and binary forms are permitted
* provided that this entire copyright notice is duplicated in all such
* copies.
*
* This software is provided "as is" and without any expressed or implied
* warranties, including, without limitation, the implied warranties of
* merchantibility and fitness for any particular purpose.
************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef USE_PAM
#ident "$Id$"

    /*
     * This module implements a simple but effective form of login access
     * control based on login names and on host (or domain) names, internet
     * addresses (or network numbers), or on terminal line names in case of
     * non-networked logins. Diagnostics are reported through syslog(3).
     *
     * Author: Wietse Venema, Eindhoven University of Technology, The Netherlands.
     */
#include <arpa/inet.h>		/* for inet_ntoa() */
#include <ctype.h>
#include <errno.h>
#include <grp.h>
#include <netdb.h>
#include <netinet/in.h>
#ifdef PRIMARY_GROUP_MATCH
#include <pwd.h>
#endif
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

#include "defines.h"
#include "prototypes.h"
#include "sizeof.h"
#include "string/strcmp/strcaseeq.h"
#include "string/strcmp/streq.h"
#include "string/strcmp/strprefix.h"
#include "string/strspn/stprspn.h"
#include "string/strtok/stpsep.h"


 /* Path name of the access control file. */
#ifndef	TABLE
#define TABLE	"/etc/login.access"
#endif

static bool list_match (char *list, const char *item, bool (*match_fn) (char *, const char *));
static bool user_match (char *tok, const char *string);
static bool from_match (char *tok, const char *string);
static bool string_match (const char *tok, const char *string);
static const char *resolve_hostname (const char *string);

/* login_access - match username/group and host/tty with access control file */
int
login_access(const char *user, const char *from)
{
	FILE *fp;
	char line[BUFSIZ];
	char *perm;		/* becomes permission field */
	char *users;		/* becomes list of login names */
	char *froms;		/* becomes list of terminals or hosts */
	bool match = false;

	/*
	 * Process the table one line at a time and stop at the first match.
	 * Blank lines and lines that begin with a '#' character are ignored.
	 * Non-comment lines are broken at the ':' character. All fields are
	 * mandatory. The first field should be a "+" or "-" character. A
	 * non-existing table means no access control.
	 */
	fp = fopen (TABLE, "r");
	if (NULL != fp) {
		intmax_t lineno = 0;	/* for diagnostics */
		while (   !match
		       && (fgets (line, sizeof (line), fp) == line))
		{
			char  *p;

			lineno++;
			if (stpsep(line, "\n") == NULL) {
				SYSLOG ((LOG_ERR,
					 "%s: line %jd: missing newline or line too long",
					 TABLE, lineno));
				continue;
			}
			if (strprefix(line, "#")) {
				continue;	/* comment line */
			}
			stpcpy(stprspn(line, " \t"), "");
			if (streq(line, "")) {	/* skip blank lines */
				continue;
			}
			p = line;
			perm = strsep(&p, ":");
			users = strsep(&p, ":");
			froms = strsep(&p, ":");
			if (froms == NULL || p != NULL) {
				SYSLOG ((LOG_ERR,
					 "%s: line %jd: bad field count",
					 TABLE, lineno));
				continue;
			}
			if (perm[0] != '+' && perm[0] != '-') {
				SYSLOG ((LOG_ERR,
					 "%s: line %jd: bad first field",
					 TABLE, lineno));
				continue;
			}
			match = (   list_match (froms, from, from_match)
			         && list_match (users, user, user_match));
		}
		(void) fclose (fp);
	} else if (errno != ENOENT) {
		int err = errno;
		SYSLOG ((LOG_ERR, "cannot open %s: %s", TABLE, strerror (err)));
	}
	return (!match || strprefix(line, "+"))?1:0;
}

/* list_match - match an item against a list of tokens with exceptions */
static bool
list_match(char *list, const char *item, bool (*match_fn)(char *, const char*))
{
	char *tok;
	bool inclusion = true;
	bool matched = false;
	bool result = false;

	/*
	 * Process tokens one at a time. We have exhausted all possible matches
	 * when we reach an "EXCEPT" token or the end of the list. If we do find
	 * a match, look for an "EXCEPT" list and determine whether the match is
	 * affected by any exceptions.
	 */
	while (NULL != (tok = strsep(&list, ", \t"))) {
		if (strcaseeq(tok, "EXCEPT")) {  /* EXCEPT: invert */
			if (!matched) {	/* stop processing: not part of list */
				break;
			}
			inclusion = !inclusion;
			matched = false;

		} else if ((*match_fn)(tok, item)) {
			result = inclusion;
			matched = true;
		}
	}

	return result;
}

/* myhostname - figure out local machine name */
static char *myhostname (void)
{
	static char name[MAXHOSTNAMELEN + 1] = "";

	if (streq(name, "")) {
		gethostname (name, sizeof (name));
		stpcpy(&name[MAXHOSTNAMELEN], "");
	}
	return (name);
}

#if HAVE_INNETGR
/* netgroup_match - match group against machine or user */
static bool
netgroup_match (const char *group, const char *machine, const char *user)
{
	static char *mydomain = NULL;

	if (mydomain == NULL) {
		static char domain[MAXHOSTNAMELEN + 1];

		getdomainname (domain, MAXHOSTNAMELEN);
		mydomain = domain;
	}

	return (innetgr (group, machine, user, mydomain) != 0);
}
#endif

/* user_match - match a username against one token */
static bool user_match (char *tok, const char *string)
{
	struct group *group;

#ifdef PRIMARY_GROUP_MATCH
	struct passwd *userinf;
#endif
	char *host;

	/*
	 * If a token has the magic value "ALL" the match always succeeds.
	 * Otherwise, return true if the token fully matches the username, or if
	 * the token is a group that contains the username.
	 */
	host = stpsep(tok + 1, "@");	/* split user@host pattern */
	if (host != NULL) {
		return user_match(tok, string) && from_match(host, myhostname());
#if HAVE_INNETGR
	} else if (strprefix(tok, "@")) {	/* netgroup */
		return (netgroup_match (tok + 1, NULL, string));
#endif
	} else if (string_match (tok, string)) {	/* ALL or exact match */
		return true;
	/* local, no need for xgetgrnam */
	} else if ((group = getgrnam (tok)) != NULL) {	/* try group membership */
		int i;
		for (i = 0; NULL != group->gr_mem[i]; i++) {
			if (strcaseeq(string, group->gr_mem[i])) {
				return true;
			}
		}
#ifdef PRIMARY_GROUP_MATCH
		/*
		 * If the string is a user whose initial GID matches the token,
		 * accept it. May avoid excessively long lines in /etc/group.
		 * Radu-Adrian Feurdean <raf@licj.soroscj.ro>
		 *
		 * XXX - disabled by default for now.  Need to verify that
		 * getpwnam() doesn't have some nasty side effects.  --marekm
		 */
		/* local, no need for xgetpwnam */
		userinf = getpwnam (string);
		if (NULL != userinf) {
			if (userinf->pw_gid == group->gr_gid) {
				return true;
			}
		}
#endif
	}
	return false;
}

static const char *resolve_hostname (const char *string)
{
	int              gai_err;
	const char       *addr_str;
	struct addrinfo  *addrs;

	static char      host[NI_MAXHOST];

	gai_err = getaddrinfo(string, NULL, NULL, &addrs);
	if (gai_err != 0) {
		SYSLOG ((LOG_ERR, "getaddrinfo(%s): %s", string, gai_strerror(gai_err)));
		return string;
	}

	addr_str = host;
	gai_err = getnameinfo(addrs[0].ai_addr, addrs[0].ai_addrlen,
	                      host, countof(host), NULL, 0, NI_NUMERICHOST);
	if (gai_err != 0) {
		SYSLOG ((LOG_ERR, "getnameinfo(%s): %s", string, gai_strerror(gai_err)));
		addr_str = string;
	}

	freeaddrinfo(addrs);
	return addr_str;
}

/* from_match - match a host or tty against a list of tokens */

static bool from_match (char *tok, const char *string)
{
	/*
	 * If a token has the magic value "ALL" the match always succeeds. Return
	 * true if the token fully matches the string. If the token is a domain
	 * name, return true if it matches the last fields of the string. If the
	 * token has the magic value "LOCAL", return true if the string does not
	 * contain a "." character. If the token is a network number, return true
	 * if it matches the head of the string.
	 */
#if HAVE_INNETGR
	if (strprefix(tok, "@")) {  /* netgroup */
		return (netgroup_match (tok + 1, string, NULL));
	} else
#endif
	if (string_match (tok, string)) {	/* ALL or exact match */
		return true;
	} else if (strprefix(tok, ".")) {  /* domain: match last fields */
		size_t  str_len, tok_len;

		str_len = strlen (string);
		tok_len = strlen (tok);
		if (   (str_len > tok_len)
		    && strcaseeq(tok, string + str_len - tok_len)) {
			return true;
		}
	} else if (strcaseeq(tok, "LOCAL")) {	/* LOCAL: no dots */
		if (strchr (string, '.') == NULL) {
			return true;
		}
	} else if (   (!streq(tok, "") && tok[strlen(tok) - 1] == '.') /* network */
		   && strprefix(resolve_hostname(string), tok)) {
		return true;
	}
	return false;
}

/* string_match - match a string against one token */
static bool string_match (const char *tok, const char *string)
{

	/*
	 * If the token has the magic value "ALL" the match always succeeds.
	 * Otherwise, return true if the token fully matches the string.
	 */
	if (strcaseeq(tok, "ALL")) {  /* ALL: always matches */
		return true;
	} else if (strcaseeq(tok, string)) {  /* try exact match */
		return true;
	}
	return false;
}

#else				/* !USE_PAM */
extern int ISO_C_forbids_an_empty_translation_unit;
#endif				/* !USE_PAM */
