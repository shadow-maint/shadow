/*
 * Copyright 1991 - 1993, Julianne Frances Haugh
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

#include <config.h>

#include "rcsid.h"
RCSID(PKG_VER "$Id: groupadd.c,v 1.14 1999/06/07 16:40:45 marekm Exp $")

#include <sys/types.h>
#include <stdio.h>
#include <grp.h>
#include <ctype.h>
#include <fcntl.h>

#include "defines.h"
#include "prototypes.h"
#include "chkname.h"

#include "getdef.h"

#include "groupio.h"

#ifdef	SHADOWGRP
#include "sgroupio.h"

static int is_shadow_grp;
#endif

/*
 * exit status values
 */

#define E_SUCCESS	0	/* success */
#define E_USAGE		2	/* invalid command syntax */
#define E_BAD_ARG	3	/* invalid argument to option */
#define E_GID_IN_USE	4	/* gid not unique (when -o not used) */
#define E_NAME_IN_USE	9	/* group name nut unique */
#define E_GRP_UPDATE	10	/* can't update group file */

static char	*group_name;
static gid_t	group_id;
static char *empty_list = NULL;

static char	*Prog;

static int oflg = 0; /* permit non-unique group ID to be specified with -g */
static int gflg = 0; /* ID value for the new group */
static int fflg = 0; /* if group already exists, do nothing and exit(0) */

#ifdef	NDBM
extern	int	gr_dbm_mode;
extern	int	sg_dbm_mode;
#endif

extern int optind;
extern char *optarg;

/* local function prototypes */
static void usage P_((void));
static void new_grent P_((struct group *));
#ifdef SHADOWGRP
static void new_sgent P_((struct sgrp *));
#endif
static void grp_update P_((void));
static void find_new_gid P_((void));
static void check_new_name P_((void));
static void process_flags P_((int, char **));
static void close_files P_((void));
static void open_files P_((void));
static void fail_exit P_((int));
int main P_((int, char **));

/*
 * usage - display usage message and exit
 */

static void
usage(void)
{
	fprintf(stderr, _("usage: groupadd [-g gid [-o]] group\n"));
	exit(E_USAGE);
}

/*
 * new_grent - initialize the values in a group file entry
 *
 *	new_grent() takes all of the values that have been entered and
 *	fills in a (struct group) with them.
 */

static void
new_grent(struct group *grent)
{
	memzero(grent, sizeof *grent);
	grent->gr_name = group_name;
	grent->gr_passwd = SHADOW_PASSWD_STRING;  /* XXX warning: const */
	grent->gr_gid = group_id;
	grent->gr_mem = &empty_list;
}

#ifdef	SHADOWGRP
/*
 * new_sgent - initialize the values in a shadow group file entry
 *
 *	new_sgent() takes all of the values that have been entered and
 *	fills in a (struct sgrp) with them.
 */

static void
new_sgent(struct sgrp *sgent)
{
	memzero(sgent, sizeof *sgent);
	sgent->sg_name = group_name;
	sgent->sg_passwd = "!";  /* XXX warning: const */
	sgent->sg_adm = &empty_list;
	sgent->sg_mem = &empty_list;
}
#endif	/* SHADOWGRP */

/*
 * grp_update - add new group file entries
 *
 *	grp_update() writes the new records to the group files.
 */

static void
grp_update(void)
{
	struct	group	grp;
#ifdef	SHADOWGRP
	struct	sgrp	sgrp;
#endif	/* SHADOWGRP */

	/*
	 * Create the initial entries for this new group.
	 */

	new_grent (&grp);
#ifdef	SHADOWGRP
	new_sgent (&sgrp);
#endif	/* SHADOWGRP */

	/*
	 * Write out the new group file entry.
	 */

	if (! gr_update (&grp)) {
		fprintf(stderr, _("%s: error adding new group entry\n"), Prog);
		fail_exit(E_GRP_UPDATE);
	}
#ifdef	NDBM

	/*
	 * Update the DBM group file with the new entry as well.
	 */

	if (gr_dbm_present() && ! gr_dbm_update (&grp)) {
		fprintf(stderr, _("%s: cannot add new dbm group entry\n"), Prog);
		fail_exit(E_GRP_UPDATE);
	}
	endgrent ();
#endif	/* NDBM */

#ifdef	SHADOWGRP

	/*
	 * Write out the new shadow group entries as well.
	 */

	if (is_shadow_grp && ! sgr_update (&sgrp)) {
		fprintf(stderr, _("%s: error adding new group entry\n"), Prog);
		fail_exit(E_GRP_UPDATE);
	}
#ifdef	NDBM

	/*
	 * Update the DBM group file with the new entry as well.
	 */

	if (is_shadow_grp && sg_dbm_present() && ! sg_dbm_update (&sgrp)) {
		fprintf(stderr, _("%s: cannot add new dbm group entry\n"), Prog);
		fail_exit(E_GRP_UPDATE);
	}
	endsgent ();
#endif	/* NDBM */
#endif	/* SHADOWGRP */
	SYSLOG((LOG_INFO, "new group: name=%s, gid=%d\n",
		group_name, group_id));
}

/*
 * find_new_gid - find the next available GID
 *
 *	find_new_gid() locates the next highest unused GID in the group
 *	file, or checks the given group ID against the existing ones for
 *	uniqueness.
 */

static void
find_new_gid(void)
{
	const struct group *grp;
	gid_t gid_min, gid_max;

	gid_min = getdef_num("GID_MIN", 100);
	gid_max = getdef_num("GID_MAX", 60000);

	/*
	 * Start with some GID value if the user didn't provide us with
	 * one already.
	 */

	if (! gflg)
		group_id = gid_min;

	/*
	 * Search the entire group file, either looking for this
	 * GID (if the user specified one with -g) or looking for the
	 * largest unused value.
	 */

#ifdef NO_GETGRENT
	gr_rewind();
	while ((grp = gr_next())) {
#else
	setgrent();
	while ((grp = getgrent())) {
#endif
		if (strcmp(group_name, grp->gr_name) == 0) {
			if (fflg) {
				fail_exit(E_SUCCESS);
			}
			fprintf(stderr, _("%s: name %s is not unique\n"),
				Prog, group_name);
			fail_exit(E_NAME_IN_USE);
		}
		if (gflg && group_id == grp->gr_gid) {
			if (fflg) {
				/* turn off -g and search again */
				gflg = 0;
#ifdef NO_GETGRENT
				gr_rewind();
#else
				setgrent();
#endif
				continue;
			}
			fprintf(stderr, _("%s: gid %ld is not unique\n"),
				Prog, (long) group_id);
			fail_exit(E_GID_IN_USE);
		}
		if (! gflg && grp->gr_gid >= group_id) {
			if (grp->gr_gid > gid_max)
				continue;
			group_id = grp->gr_gid + 1;
		}
	}
	if (!gflg && group_id == gid_max + 1) {
		for (group_id = gid_min; group_id < gid_max; group_id++) {
#ifdef NO_GETGRENT
			gr_rewind();
			while ((grp = gr_next()) && grp->gr_gid != group_id)
				;
			if (!grp)
				break;
#else
			if (!getgrgid(group_id))
				break;
#endif
		}
		if (group_id == gid_max) {
			fprintf(stderr, _("%s: can't get unique gid\n"),
				Prog);
			fail_exit(E_GID_IN_USE);
		}
	}
}

/*
 * check_new_name - check the new name for validity
 *
 *	check_new_name() insures that the new name doesn't contain
 *	any illegal characters.
 */

static void
check_new_name(void)
{
	if (check_group_name(group_name))
		return;

	/*
	 * All invalid group names land here.
	 */

	fprintf(stderr, _("%s: %s is a not a valid group name\n"),
		Prog, group_name);

	exit(E_BAD_ARG);
}

/*
 * process_flags - perform command line argument setting
 *
 *	process_flags() interprets the command line arguments and sets
 *	the values that the user will be created with accordingly.  The
 *	values are checked for sanity.
 */

static void
process_flags(int argc, char **argv)
{
	char *cp;
	int arg;

	while ((arg = getopt(argc, argv, "og:O:f")) != EOF) {
		switch (arg) {
		case 'g':
			gflg++;
			if (! isdigit (optarg[0]))
				usage ();

			group_id = strtol(optarg, &cp, 10);
			if (*cp != '\0') {
				fprintf(stderr, _("%s: invalid group %s\n"),
					Prog, optarg);
				fail_exit(E_BAD_ARG);
			}
			break;
		case 'o':
			oflg++;
			break;
		case 'O':
			/*
			 * override login.defs defaults (-O name=value)
			 * example: -O GID_MIN=100 -O GID_MAX=499
			 * note: -O GID_MIN=10,GID_MAX=499 doesn't work yet
			 */
			cp = strchr(optarg, '=');
			if (!cp) {
				fprintf(stderr,
					_("%s: -O requires NAME=VALUE\n"),
					Prog);
				exit(E_BAD_ARG);
			}
			/* terminate name, point to value */
			*cp++ = '\0';
			if (putdef_str(optarg, cp) < 0)
				exit(E_BAD_ARG);
			break;
		case 'f':
			/*
			 * "force" - do nothing, just exit(0), if the
			 * specified group already exists.  With -g, if
			 * specified gid already exists, choose another
			 * (unique) gid (turn off -g).  Based on the
			 * RedHat's patch from shadow-utils-970616-9.
			 */
			fflg++;
			break;
		default:
			usage();
		}
	}

	if (oflg && !gflg)
		usage();

	if (optind != argc - 1)
		usage();

	group_name = argv[argc - 1];
	check_new_name();
}

/*
 * close_files - close all of the files that were opened
 *
 *	close_files() closes all of the files that were opened for this
 *	new group.  This causes any modified entries to be written out.
 */

static void
close_files(void)
{
	if (!gr_close()) {
		fprintf(stderr, _("%s: cannot rewrite group file\n"), Prog);
		fail_exit(E_GRP_UPDATE);
	}
	gr_unlock();
#ifdef	SHADOWGRP
	if (is_shadow_grp && !sgr_close()) {
		fprintf(stderr, _("%s: cannot rewrite shadow group file\n"),
			Prog);
		fail_exit(E_GRP_UPDATE);
	}
	if (is_shadow_grp)
		sgr_unlock ();
#endif	/* SHADOWGRP */
}

/*
 * open_files - lock and open the group files
 *
 *	open_files() opens the two group files.
 */

static void
open_files(void)
{
	if (! gr_lock ()) {
		fprintf(stderr, _("%s: unable to lock group file\n"), Prog);
		exit(E_GRP_UPDATE);
	}
	if (! gr_open (O_RDWR)) {
		fprintf(stderr, _("%s: unable to open group file\n"), Prog);
		fail_exit(E_GRP_UPDATE);
	}
#ifdef	SHADOWGRP
	if (is_shadow_grp && ! sgr_lock ()) {
		fprintf(stderr, _("%s: unable to lock shadow group file\n"),
			Prog);
		fail_exit(E_GRP_UPDATE);
	}
	if (is_shadow_grp && ! sgr_open (O_RDWR)) {
		fprintf(stderr, _("%s: unable to open shadow group file\n"),
			Prog);
		fail_exit(E_GRP_UPDATE);
	}
#endif	/* SHADOWGRP */
}

/*
 * fail_exit - exit with an error code after unlocking files
 */

static void
fail_exit(int code)
{
	(void) gr_unlock ();
#ifdef	SHADOWGRP
	if (is_shadow_grp)
		sgr_unlock ();
#endif
	exit (code);
}

/*
 * main - groupadd command
 */

int
main(int argc, char **argv)
{

	/*
	 * Get my name so that I can use it to report errors.
	 */

	Prog = Basename(argv[0]);

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	openlog(Prog, LOG_PID|LOG_CONS|LOG_NOWAIT, LOG_AUTH);

#ifdef SHADOWGRP
	is_shadow_grp = sgr_file_present();
#endif

	/*
	 * The open routines for the DBM files don't use read-write
	 * as the mode, so we have to clue them in.
	 */

#ifdef	NDBM
	gr_dbm_mode = O_RDWR;
#ifdef	SHADOWGRP
	sg_dbm_mode = O_RDWR;
#endif	/* SHADOWGRP */
#endif	/* NDBM */
	process_flags(argc, argv);

	/*
	 * Start with a quick check to see if the group exists.
	 */

	if (getgrnam(group_name)) {
		if (fflg) {
			exit(E_SUCCESS);
		}
		fprintf(stderr, _("%s: group %s exists\n"), Prog, group_name);
		exit(E_NAME_IN_USE);
	}

	/*
	 * Do the hard stuff - open the files, create the group entries,
	 * then close and update the files.
	 */

	open_files();

	if (!gflg || !oflg)
		find_new_gid();

	grp_update();

	close_files();
	exit(E_SUCCESS);
	/*NOTREACHED*/
}
