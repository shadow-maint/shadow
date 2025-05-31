/*
 * SPDX-FileCopyrightText: 2011       , Julian Pidancet
 * SPDX-FileCopyrightText: 2011       , Nicolas François
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <config.h>

#ident "$Id$"

#include <stdio.h>
#include <assert.h>

#include "atoi/getnum.h"
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
#include "shadowlog.h"
#include "string/sprintf/xaprintf.h"
#include "string/strcmp/streq.h"
#include "string/strcmp/strprefix.h"


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
	const char *prefix = NULL;

	for (int i = 0; i < argc; i++) {
		const char  *val;

		val = strprefix(argv[i], "--prefix=");

		if (   streq(argv[i], "--prefix")
		    || val != NULL
		    || streq(argv[i], short_opt))
		{
			if (NULL != prefix) {
				fprintf (log_get_logfd(),
				         _("%s: multiple --prefix options\n"),
				         log_get_progname());
				exit (E_BAD_ARG);
			}

			if (val) {
				prefix = val;
			} else if (i + 1 == argc) {
				fprintf (log_get_logfd(),
				         _("%s: option '%s' requires an argument\n"),
				         log_get_progname(), argv[i]);
				exit (E_BAD_ARG);
			} else {
				prefix = argv[++ i];
			}
		}
	}



	if (prefix != NULL) {
		/* Drop privileges */
		if (   (setregid (getgid (), getgid ()) != 0)
		    || (setreuid (getuid (), getuid ()) != 0)) {
			fprintf (log_get_logfd(),
			         _("%s: failed to drop privileges (%s)\n"),
			         log_get_progname(), strerror (errno));
			exit (EXIT_FAILURE);
		}

		if (streq(prefix, "") || streq(prefix, "/"))
			return ""; /* if prefix is "/" then we ignore the flag option */
		/* should we prevent symbolic link from being used as a prefix? */

		if ( prefix[0] != '/') {
			fprintf (log_get_logfd(),
				 _("%s: prefix must be an absolute path\n"),
				 log_get_progname());
			exit (E_BAD_ARG);
		}

		passwd_db_file = xaprintf("%s/%s", prefix, PASSWD_FILE);
		pw_setdbname(passwd_db_file);

		group_db_file = xaprintf("%s/%s", prefix, GROUP_FILE);
		gr_setdbname(group_db_file);

#ifdef  SHADOWGRP
		sgroup_db_file = xaprintf("%s/%s", prefix, SGROUP_FILE);
		sgr_setdbname(sgroup_db_file);
#endif

		spw_db_file = xaprintf("%s/%s", prefix, SHADOW_FILE);
		spw_setdbname(spw_db_file);

#ifdef ENABLE_SUBIDS
		suid_db_file = xaprintf("%s/%s", prefix, SUBUID_FILE);
		sub_uid_setdbname(suid_db_file);

		sgid_db_file = xaprintf("%s/%s", prefix, SUBGID_FILE);
		sub_gid_setdbname(sgid_db_file);
#endif

#ifdef USE_ECONF
		setdef_config_file(prefix);
#else
		def_conf_file = xaprintf("%s/%s", prefix, "/etc/login.defs");
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
		if (!fg)
			return NULL;
		while ((grp = fgetgrent(fg)) != NULL) {
			if (streq(name, grp->gr_name))
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
		if (!fg)
			return NULL;
		while ((grp = fgetgrent(fg)) != NULL) {
			if (gid == grp->gr_gid)
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
		if (!fg)
			return NULL;
		while ((pwd = fgetpwent(fg)) != NULL) {
			if (uid == pwd->pw_uid)
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
		if (!fg)
			return NULL;
		while ((pwd = fgetpwent(fg)) != NULL) {
			if (streq(name, pwd->pw_name))
				break;
		}
		fclose(fg);
		return pwd;
	}
	else {
		return getpwnam(name);
	}
}
#if HAVE_FGETPWENT_R
extern int prefix_getpwnam_r(const char* name, struct passwd* pwd,
                             char* buf, size_t buflen, struct passwd** result)
{
	if (passwd_db_file) {
		FILE* fg;
		int ret = 0;

		fg = fopen(passwd_db_file, "rt");
		if (!fg)
			return errno;
		while ((ret = fgetpwent_r(fg, pwd, buf, buflen, result)) == 0) {
			if (streq(name, pwd->pw_name))
				break;
		}
		fclose(fg);
		return ret;
	}
	else {
		return getpwnam_r(name, pwd, buf, buflen, result);
	}
}
#endif
extern struct spwd *prefix_getspnam(const char* name)
{
	if (spw_db_file) {
		FILE* fg;
		struct spwd *sp = NULL;

		fg = fopen(spw_db_file, "rt");
		if (!fg)
			return NULL;
		while ((sp = fgetspent(fg)) != NULL) {
			if (streq(name, sp->sp_namp))
				break;
		}
		fclose(fg);
		return sp;
	}
	else {
		return getspnam(name);
	}
}

extern void prefix_setpwent(void)
{
	if (!passwd_db_file) {
		setpwent();
		return;
	}
	if (fp_pwent)
		fclose (fp_pwent);

	fp_pwent = fopen(passwd_db_file, "rt");
	if (!fp_pwent)
		return;
}
extern struct passwd* prefix_getpwent(void)
{
	if (!passwd_db_file) {
		return getpwent();
	}
	if (!fp_pwent) {
		return NULL;
	}
	return fgetpwent(fp_pwent);
}
extern void prefix_endpwent(void)
{
	if (!passwd_db_file) {
		endpwent();
		return;
	}
	if (fp_pwent)
		fclose(fp_pwent);
	fp_pwent = NULL;
}

extern void prefix_setgrent(void)
{
	if (!group_db_file) {
		setgrent();
		return;
	}
	if (fp_grent)
		fclose (fp_grent);

	fp_grent = fopen(group_db_file, "rt");
	if (!fp_grent)
		return;
}
extern struct group* prefix_getgrent(void)
{
	if (!group_db_file) {
		return getgrent();
	}
	return fgetgrent(fp_grent);
}
extern void prefix_endgrent(void)
{
	if (!group_db_file) {
		endgrent();
		return;
	}
	if (fp_grent)
		fclose(fp_grent);
	fp_grent = NULL;
}

extern struct group *prefix_getgr_nam_gid(const char *grname)
{
	gid_t         gid;
	struct group  *g;

	if (NULL == grname) {
		return NULL;
	}

	if (!group_db_file)
		return getgr_nam_gid(grname);

	if (get_gid(grname, &gid) == 0)
		return prefix_getgrgid(gid);

	g = prefix_getgrnam(grname);
	return g ? __gr_dup(g) : NULL;
}
