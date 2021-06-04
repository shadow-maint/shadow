/*
 * Copyright (c) 2011       , Julian Pidancet
 * Copyright (c) 2011       , Nicolas François
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
 * 3. The name of the copyright holders or contributors may not be used to
 *    endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <config.h>

#ident "$Id$"

#include <stdio.h>
#include <assert.h>
#include "defines.h"
#include "prototypes.h"
/*@-exitarg@*/
#include "exitcodes.h"
#include "groupio.h"
#include "pwio.h"
#ifdef	SHADOWGRP
#include "sgroupio.h"
#endif
#include "shadowio.h"
#ifdef ENABLE_SUBIDS
#include "subordinateio.h"
#endif				/* ENABLE_SUBIDS */
#include "getdef.h"

static char *passwd_db_file = NULL;
static char *spw_db_file = NULL;
static char *group_db_file = NULL;
static char *sgroup_db_file = NULL;
static char *suid_db_file = NULL;
static char *sgid_db_file = NULL;
static char *def_conf_file = NULL;
static FILE* fp_pwent = NULL;
static FILE* fp_grent = NULL;

/*
 * process_prefix_flag - prefix all paths if given the --prefix option
 *
 * This shall be called before accessing the passwd, group, shadow,
 * gshadow, useradd's default, login.defs files (non exhaustive list)
 * or authenticating the caller.
 *
 * The audit, syslog, or locale files shall be open before
 */
extern const char* process_prefix_flag (const char* short_opt, int argc, char **argv)
{
	/*
	 * Parse the command line options.
	 */
	int i;
	const char *prefix = NULL, *val;

	for (i = 0; i < argc; i++) {
		val = NULL;
		if (   (strcmp (argv[i], "--prefix") == 0)
		    || ((strncmp (argv[i], "--prefix=", 9) == 0)
			&& (val = argv[i] + 9))
		    || (strcmp (argv[i], short_opt) == 0)) {
			if (NULL != prefix) {
				fprintf (shadow_logfd,
				         _("%s: multiple --prefix options\n"),
				         Prog);
				exit (E_BAD_ARG);
			}

			if (val) {
				prefix = val;
			} else if (i + 1 == argc) {
				fprintf (shadow_logfd,
				         _("%s: option '%s' requires an argument\n"),
				         Prog, argv[i]);
				exit (E_BAD_ARG);
			} else {
				prefix = argv[++ i];
			}
		}
	}



	if (prefix != NULL) {
		if ( prefix[0] == '\0' || !strcmp(prefix, "/"))
			return ""; /* if prefix is "/" then we ignore the flag option */
		/* should we prevent symbolic link from being used as a prefix? */

		if ( prefix[0] != '/') {
			fprintf (shadow_logfd,
				 _("%s: prefix must be an absolute path\n"),
				 Prog);
			exit (E_BAD_ARG);
		}
		size_t len;
		len = strlen(prefix) + strlen(PASSWD_FILE) + 2;
		passwd_db_file = xmalloc(len);
		snprintf(passwd_db_file, len, "%s/%s", prefix, PASSWD_FILE);
		pw_setdbname(passwd_db_file);

		len = strlen(prefix) + strlen(GROUP_FILE) + 2;
		group_db_file = xmalloc(len);
		snprintf(group_db_file, len, "%s/%s", prefix, GROUP_FILE);
		gr_setdbname(group_db_file);

#ifdef  SHADOWGRP
		len = strlen(prefix) + strlen(SGROUP_FILE) + 2;
		sgroup_db_file = xmalloc(len);
		snprintf(sgroup_db_file, len, "%s/%s", prefix, SGROUP_FILE);
		sgr_setdbname(sgroup_db_file);
#endif
#ifdef	USE_NIS
		__setspNIS(0); /* disable NIS for now, at least until it is properly supporting a "prefix" */
#endif

		len = strlen(prefix) + strlen(SHADOW_FILE) + 2;
		spw_db_file = xmalloc(len);
		snprintf(spw_db_file, len, "%s/%s", prefix, SHADOW_FILE);
		spw_setdbname(spw_db_file);

#ifdef ENABLE_SUBIDS
		len = strlen(prefix) + strlen("/etc/subuid") + 2;
		suid_db_file = xmalloc(len);
		snprintf(suid_db_file, len, "%s/%s", prefix, "/etc/subuid");
		sub_uid_setdbname(suid_db_file);

		len = strlen(prefix) + strlen("/etc/subgid") + 2;
		sgid_db_file = xmalloc(len);
		snprintf(sgid_db_file, len, "%s/%s", prefix, "/etc/subgid");
		sub_gid_setdbname(sgid_db_file);
#endif

#ifdef USE_ECONF
		setdef_config_file(prefix);
#else
		len = strlen(prefix) + strlen("/etc/login.defs") + 2;
		def_conf_file = xmalloc(len);
		snprintf(def_conf_file, len, "%s/%s", prefix, "/etc/login.defs");
		setdef_config_file(def_conf_file);
#endif
	}

	if (prefix == NULL)
		return "";
	return prefix;
}


extern struct group *prefix_getgrnam(const char *name)
{
	if (group_db_file) {
		FILE* fg;
		struct group * grp = NULL;

		fg = fopen(group_db_file, "rt");
		if(!fg)
			return NULL;
		while((grp = fgetgrent(fg)) != NULL) {
			if(!strcmp(name, grp->gr_name))
				break;
		}
		fclose(fg);
		return grp;
	}

	return getgrnam(name);
}

extern struct group *prefix_getgrgid(gid_t gid)
{
	if (group_db_file) {
		FILE* fg;
		struct group * grp = NULL;

		fg = fopen(group_db_file, "rt");
		if(!fg)
			return NULL;
		while((grp = fgetgrent(fg)) != NULL) {
			if(gid == grp->gr_gid)
				break;
		}
		fclose(fg);
		return grp;
	}

	return getgrgid(gid);
}

extern struct passwd *prefix_getpwuid(uid_t uid)
{
	if (passwd_db_file) {
		FILE* fg;
		struct passwd *pwd = NULL;

		fg = fopen(passwd_db_file, "rt");
		if(!fg)
			return NULL;
		while((pwd = fgetpwent(fg)) != NULL) {
			if(uid == pwd->pw_uid)
				break;
		}
		fclose(fg);
		return pwd;
	}
	else {
		return getpwuid(uid);
	}
}
extern struct passwd *prefix_getpwnam(const char* name)
{
	if (passwd_db_file) {
		FILE* fg;
		struct passwd *pwd = NULL;

		fg = fopen(passwd_db_file, "rt");
		if(!fg)
			return NULL;
		while((pwd = fgetpwent(fg)) != NULL) {
			if(!strcmp(name, pwd->pw_name))
				break;
		}
		fclose(fg);
		return pwd;
	}
	else {
		return getpwnam(name);
	}
}
extern struct spwd *prefix_getspnam(const char* name)
{
	if (spw_db_file) {
		FILE* fg;
		struct spwd *sp = NULL;

		fg = fopen(spw_db_file, "rt");
		if(!fg)
			return NULL;
		while((sp = fgetspent(fg)) != NULL) {
			if(!strcmp(name, sp->sp_namp))
				break;
		}
		fclose(fg);
		return sp;
	}
	else {
		return getspnam(name);
	}
}

extern void prefix_setpwent()
{
	if(!passwd_db_file) {
		setpwent();
		return;
	}
	if (fp_pwent)
		fclose (fp_pwent);

	fp_pwent = fopen(passwd_db_file, "rt");
	if(!fp_pwent)
		return;
}
extern struct passwd* prefix_getpwent()
{
	if(!passwd_db_file) {
		return getpwent();
	}
	return fgetpwent(fp_pwent);
}
extern void prefix_endpwent()
{
	if(!passwd_db_file) {
		endpwent();
		return;
	}
	if (fp_pwent)
		fclose(fp_pwent);
	fp_pwent = NULL;
}

extern void prefix_setgrent()
{
	if(!group_db_file) {
		setgrent();
		return;
	}
	if (fp_grent)
		fclose (fp_grent);

	fp_grent = fopen(group_db_file, "rt");
	if(!fp_grent)
		return;
}
extern struct group* prefix_getgrent()
{
	if(!group_db_file) {
		return getgrent();
	}
	return fgetgrent(fp_grent);
}
extern void prefix_endgrent()
{
	if(!group_db_file) {
		endgrent();
		return;
	}
	if (fp_grent)
		fclose(fp_grent);
	fp_grent = NULL;
}

extern struct group *prefix_getgr_nam_gid(const char *grname)
{
	long long int gid;
	char *endptr;
	struct group *g;

	if (NULL == grname) {
		return NULL;
	}

	if (group_db_file) {
		errno = 0;
		gid = strtoll (grname, &endptr, 10);
		if (   ('\0' != *grname)
	    	&& ('\0' == *endptr)
	    	&& (ERANGE != errno)
	    	&& (gid == (gid_t)gid)) {
			return prefix_getgrgid ((gid_t) gid);
		}
		g = prefix_getgrnam (grname);
		return g ? __gr_dup(g) : NULL;
	}
	else
		return getgr_nam_gid(grname);
}
