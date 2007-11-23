/*
 * Copyright 1990 - 1993, Julianne Frances Haugh
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
 * ARE DISCLAIMED. IN NO EVENT SHALL JULIE HAUGH OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	newusers - create users from a batch file
 *
 *	newusers creates a collection of entries in /etc/passwd
 *	and related files by reading a passwd-format file and
 *	adding entries in the related directories.
 */

#include <config.h>

#ident "$Id$"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <pwd.h>
#include <grp.h>
#include <fcntl.h>
#include <getopt.h>
#ifdef USE_PAM
#include "pam_defs.h"
#endif				/* USE_PAM */
#include "prototypes.h"
#include "defines.h"
#include "getdef.h"
#include "groupio.h"
#include "nscd.h"
#include "pwio.h"
#include "shadowio.h"
/*
 * Global variables
 */
static char *Prog;
static int cflg = 0;
static int sflg = 0;

static char *crypt_method = NULL;
static long sha_rounds = 5000;

static int is_shadow;

/* local function prototypes */
static void usage (void);
static int add_group (const char *, const char *, gid_t *);
static int add_user (const char *, const char *, uid_t *, gid_t);
static void update_passwd (struct passwd *, const char *);
static int add_passwd (struct passwd *, const char *);

/*
 * usage - display usage message and exit
 */
static void usage (void)
{
	fprintf (stderr, _("Usage: %s [options] [input]\n"
	                   "\n"
			   "  -c, --crypt-method	the crypt method (one of %s)\n"
			   "%s"
			   "\n"),
			 Prog,
#ifndef ENCRYPTMETHOD_SELECT
			 "NONE DES MD5", ""
#else
			 "NONE DES MD5 SHA256 SHA512",
			 _("  -s, --sha-rounds		number of SHA rounds for the SHA* crypt algorithms\n")
#endif
			 );
	exit (1);
}

/*
 * add_group - create a new group or add a user to an existing group
 */
static int add_group (const char *name, const char *gid, gid_t * ngid)
{
	const struct passwd *pwd;
	const struct group *grp;
	struct group grent;
	char *members[2];
	int i;

	/*
	 * Start by seeing if the named group already exists. This will be
	 * very easy to deal with if it does.
	 */
	if ((grp = gr_locate (gid))) {
	      add_member:
		grent = *grp;
		*ngid = grent.gr_gid;
		for (i = 0; grent.gr_mem[i] != (char *) 0; i++)
			if (strcmp (grent.gr_mem[i], name) == 0)
				return 0;

		grent.gr_mem = (char **) xmalloc (sizeof (char *) * (i + 2));
		memcpy (grent.gr_mem, grp->gr_mem, sizeof (char *) * (i + 2));
		grent.gr_mem[i] = xstrdup (name);
		grent.gr_mem[i + 1] = (char *) 0;

		return !gr_update (&grent);
	}

	/*
	 * The group did not exist, so I try to figure out what the GID is
	 * going to be. The gid parameter is probably "", meaning I figure
	 * out the GID from the password file. I want the UID and GID to
	 * match, unless the GID is already used.
	 */
	if (gid[0] == '\0') {
		i = 100;
		for (pw_rewind (); (pwd = pw_next ());) {
			if (pwd->pw_uid >= (unsigned int)i)
				i = pwd->pw_uid + 1;
		}
		for (gr_rewind (); (grp = gr_next ());) {
			if (grp->gr_gid == (unsigned int)i) {
				i = -1;
				break;
			}
		}
	} else if (gid[0] >= '0' && gid[0] <= '9') {
		/*
		 * The GID is a number, which means either this is a brand
		 * new group, or an existing group. For existing groups I
		 * just add myself as a member, just like I did earlier.
		 */
		i = atoi (gid);
		for (gr_rewind (); (grp = gr_next ());)
			if (grp->gr_gid == (unsigned int)i)
				goto add_member;
	} else
		/*
		 * The last alternative is that the GID is a name which is
		 * not already the name of an existing group, and I need to
		 * figure out what group ID that group name is going to
		 * have.
		 */
		i = -1;

	/*
	 * If I don't have a group ID by now, I'll go get the next one.
	 */
	if (i == -1) {
		for (i = 100, gr_rewind (); (grp = gr_next ());)
			if (grp->gr_gid >= (unsigned int)i)
				i = grp->gr_gid + 1;
	}

	/*
	 * Now I have all of the fields required to create the new group.
	 */
	if (gid[0] && (gid[0] <= '0' || gid[0] >= '9'))
		grent.gr_name = xstrdup (gid);
	else
		grent.gr_name = xstrdup (name);

	grent.gr_passwd = "x";	/* XXX warning: const */
	grent.gr_gid = i;
	members[0] = xstrdup (name);
	members[1] = (char *) 0;
	grent.gr_mem = members;

	*ngid = grent.gr_gid;
	return !gr_update (&grent);
}

/*
 * add_user - create a new user ID
 */
static int add_user (const char *name, const char *uid, uid_t * nuid, gid_t gid)
{
	const struct passwd *pwd;
	struct passwd pwent;
	uid_t i;

	/*
	 * The first guess for the UID is either the numerical UID that the
	 * caller provided, or the next available UID.
	 */
	if (uid[0] >= '0' && uid[0] <= '9') {
		i = atoi (uid);
	} else if (uid[0] && (pwd = pw_locate (uid))) {
		i = pwd->pw_uid;
	} else {
		/* Start with gid, either the specified GID, or an ID
		 * greater than all the group and user IDs */
		i = gid;
		for (pw_rewind (); (pwd = pw_next ());)
			if (pwd->pw_uid >= i)
				i = pwd->pw_uid + 1;
	}

	/*
	 * I don't want to fill in the entire password structure members
	 * JUST YET, since there is still more data to be added. So, I fill
	 * in the parts that I have.
	 */
	pwent.pw_name = xstrdup (name);
	pwent.pw_passwd = "x";	/* XXX warning: const */
	pwent.pw_uid = i;
	pwent.pw_gid = gid;
	pwent.pw_gecos = "";	/* XXX warning: const */
	pwent.pw_dir = "";	/* XXX warning: const */
	pwent.pw_shell = "";	/* XXX warning: const */

	*nuid = i;
	return !pw_update (&pwent);
}

static void update_passwd (struct passwd *pwd, const char *passwd)
{
	void *arg = NULL;
	if (crypt_method != NULL) {
		if (sflg)
			arg = &sha_rounds;
	}

	if (crypt_method != NULL && 0 == strcmp(crypt_method, "NONE")) {
		pwd->pw_passwd = (char *)passwd;
	} else {
		pwd->pw_passwd = pw_encrypt (passwd,
		                             crypt_make_salt (crypt_method,
		                                              arg));
	}
}

/*
 * add_passwd - add or update the encrypted password
 */
static int add_passwd (struct passwd *pwd, const char *passwd)
{
	const struct spwd *sp;
	struct spwd spent;

	/*
	 * In the case of regular password files, this is real easy - pwd
	 * points to the entry in the password file. Shadow files are
	 * harder since there are zillions of things to do ...
	 */
	if (!is_shadow) {
		update_passwd (pwd, passwd);
		return 0;
	}

	/*
	 * Do the first and easiest shadow file case. The user already
	 * exists in the shadow password file.
	 */
	if ((sp = spw_locate (pwd->pw_name))) {
		spent = *sp;
		spent.sp_pwdp = pw_encrypt (passwd,
		                            crypt_make_salt (NULL, NULL));
		return !spw_update (&spent);
	}

	/*
	 * Pick the next easiest case - the user has an encrypted password
	 * which isn't equal to "x". The password was set to "x" earlier
	 * when the entry was created, so this user would have to have had
	 * the password set someplace else.
	 */
	if (strcmp (pwd->pw_passwd, "x") != 0) {
		update_passwd (pwd, passwd);
		return 0;
	}

	/*
	 * Now the really hard case - I need to create an entirely new
	 * shadow password file entry.
	 */
	spent.sp_namp = pwd->pw_name;
	spent.sp_pwdp = pw_encrypt (passwd, crypt_make_salt (NULL, NULL));
	spent.sp_lstchg = time ((time_t *) 0) / SCALE;
	spent.sp_min = getdef_num ("PASS_MIN_DAYS", 0);
	/* 10000 is infinity this week */
	spent.sp_max = getdef_num ("PASS_MAX_DAYS", 10000);
	spent.sp_warn = getdef_num ("PASS_WARN_AGE", -1);
	spent.sp_inact = -1;
	spent.sp_expire = -1;
	spent.sp_flag = -1;

	return !spw_update (&spent);
}

int main (int argc, char **argv)
{
	char buf[BUFSIZ];
	char *fields[8];
	int nfields;
	char *cp;
	const struct passwd *pw;
	struct passwd newpw;
	int errors = 0;
	int line = 0;
	uid_t uid;
	gid_t gid;

#ifdef USE_PAM
	pam_handle_t *pamh = NULL;
	int retval;
#endif

	Prog = Basename (argv[0]);

	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);

	{
		int option_index = 0;
		int c;
		static struct option long_options[] = {
			{"crypt-method", required_argument, NULL, 'c'},
			{"help", no_argument, NULL, 'h'},
			{"sha-rounds", required_argument, NULL, 's'},
			{NULL, 0, NULL, '\0'}
		};

		while ((c =
			getopt_long (argc, argv, "c:hs:", long_options,
			             &option_index)) != -1) {
			switch (c) {
			case 'c':
				cflg = 1;
				crypt_method = optarg;
				break;
			case 'h':
				usage ();
				break;
			case 's':
				sflg = 1;
				if (!getlong(optarg, &sha_rounds)) {
					fprintf (stderr,
					         _("%s: invalid numeric argument '%s'\n"),
					         Prog, optarg);
					usage ();
				}
				break;
			case 0:
				/* long option */
				break;
			default:
				usage ();
				break;
			}
		}
	}

	/* validate options */
	if (sflg && !cflg) {
		fprintf (stderr,
		         _("%s: %s flag is ONLY allowed with the %s flag\n"),
		         Prog, "-s", "-c");
		usage ();
	}
	if (cflg) {
		if (0 != strcmp (crypt_method, "DES") &&
		    0 != strcmp (crypt_method, "MD5") &&
		    0 != strcmp (crypt_method, "NONE") &&
#ifdef ENCRYPTMETHOD_SELECT
		    0 != strcmp (crypt_method, "SHA256") &&
		    0 != strcmp (crypt_method, "SHA512")
#endif
		    ) {
			fprintf (stderr,
			         _("%s: unsupported crypt method: %s\n"),
			         Prog, crypt_method);
			usage ();
		}
	}

	if (argv[optind] != NULL) {
		if (!freopen (argv[optind], "r", stdin)) {
			snprintf (buf, sizeof buf, "%s: %s", Prog, argv[1]);
			perror (buf);
			exit (1);
		}
	}


#ifdef USE_PAM
	retval = PAM_SUCCESS;

	{
		struct passwd *pampw;
		pampw = getpwuid (getuid ()); /* local, no need for xgetpwuid */
		if (pampw == NULL) {
			retval = PAM_USER_UNKNOWN;
		}

		if (retval == PAM_SUCCESS) {
			retval = pam_start ("newusers", pampw->pw_name,
					    &conv, &pamh);
		}
	}

	if (retval == PAM_SUCCESS) {
		retval = pam_authenticate (pamh, 0);
		if (retval != PAM_SUCCESS) {
			pam_end (pamh, retval);
		}
	}

	if (retval == PAM_SUCCESS) {
		retval = pam_acct_mgmt (pamh, 0);
		if (retval != PAM_SUCCESS) {
			pam_end (pamh, retval);
		}
	}

	if (retval != PAM_SUCCESS) {
		fprintf (stderr, _("%s: PAM authentication failed\n"), Prog);
		exit (1);
	}
#endif				/* USE_PAM */

	/*
	 * Lock the password files and open them for update. This will bring
	 * all of the entries into memory where they may be searched for an
	 * modified, or new entries added. The password file is the key - if
	 * it gets locked, assume the others can be locked right away.
	 */
	if (!pw_lock ()) {
		fprintf (stderr, _("%s: can't lock /etc/passwd.\n"), Prog);
		exit (1);
	}
	is_shadow = spw_file_present ();

	if ((is_shadow && !spw_lock ()) || !gr_lock ()) {
		fprintf (stderr,
			 _("%s: can't lock files, try again later\n"), Prog);
		(void) pw_unlock ();
		if (is_shadow)
			spw_unlock ();
		exit (1);
	}
	if (!pw_open (O_RDWR) || (is_shadow && !spw_open (O_RDWR))
	    || !gr_open (O_RDWR)) {
		fprintf (stderr, _("%s: can't open files\n"), Prog);
		(void) pw_unlock ();
		if (is_shadow)
			spw_unlock ();
		(void) gr_unlock ();
		exit (1);
	}

	/*
	 * Read each line. The line has the same format as a password file
	 * entry, except that certain fields are not constrained to be
	 * numerical values. If a group ID is entered which does not already
	 * exist, an attempt is made to allocate the same group ID as the
	 * numerical user ID. Should that fail, the next available group ID
	 * over 100 is allocated. The pw_gid field will be updated with that
	 * value.
	 */
	while (fgets (buf, sizeof buf, stdin) != (char *) 0) {
		line++;
		if ((cp = strrchr (buf, '\n'))) {
			*cp = '\0';
		} else {
			fprintf (stderr, _("%s: line %d: line too long\n"),
				 Prog, line);
			errors++;
			continue;
		}

		/*
		 * Break the string into fields and screw around with them.
		 * There MUST be 7 colon separated fields, although the
		 * values aren't that particular.
		 */
		for (cp = buf, nfields = 0; nfields < 7; nfields++) {
			fields[nfields] = cp;
			if ((cp = strchr (cp, ':')))
				*cp++ = '\0';
			else
				break;
		}
		if (nfields != 6) {
			fprintf (stderr, _("%s: line %d: invalid line\n"),
				 Prog, line);
			continue;
		}

		/*
		 * Now the fields are processed one by one. The first field
		 * to be processed is the group name. A new group will be
		 * created if the group name is non-numeric and does not
		 * already exist. The named user will be the only member. If
		 * there is no named group to be a member of, the UID will
		 * be figured out and that value will be a candidate for a
		 * new group, if that group ID exists, a whole new group ID
		 * will be made up.
		 */
		if (!(pw = pw_locate (fields[0])) &&
		    add_group (fields[0], fields[3], &gid)) {
			fprintf (stderr,
				 _("%s: line %d: can't create GID\n"),
				 Prog, line);
			errors++;
			continue;
		}

		/*
		 * Now we work on the user ID. It has to be specified either
		 * as a numerical value, or left blank. If it is a numerical
		 * value, that value will be used, otherwise the next
		 * available user ID is computed and used. After this there
		 * will at least be a (struct passwd) for the user.
		 */
		if (!pw && add_user (fields[0], fields[2], &uid, gid)) {
			fprintf (stderr,
				 _("%s: line %d: can't create UID\n"),
				 Prog, line);
			errors++;
			continue;
		}

		/*
		 * The password, gecos field, directory, and shell fields
		 * all come next.
		 */
		if (!(pw = pw_locate (fields[0]))) {
			fprintf (stderr,
				 _("%s: line %d: cannot find user %s\n"),
				 Prog, line, fields[0]);
			errors++;
			continue;
		}
		newpw = *pw;

		if (add_passwd (&newpw, fields[1])) {
			fprintf (stderr,
				 _("%s: line %d: can't update password\n"),
				 Prog, line);
			errors++;
			continue;
		}
		if (fields[4][0])
			newpw.pw_gecos = fields[4];

		if (fields[5][0])
			newpw.pw_dir = fields[5];

		if (fields[6][0])
			newpw.pw_shell = fields[6];

		if (newpw.pw_dir[0] && access (newpw.pw_dir, F_OK)) {
			if (mkdir (newpw.pw_dir,
				   0777 & ~getdef_num ("UMASK",
						       GETDEF_DEFAULT_UMASK)))
				fprintf (stderr,
					 _("%s: line %d: mkdir failed\n"), Prog,
					 line);
			else if (chown
				 (newpw.pw_dir, newpw.pw_uid, newpw.pw_gid))
				fprintf (stderr,
					 _("%s: line %d: chown failed\n"), Prog,
					 line);
		}

		/*
		 * Update the password entry with the new changes made.
		 */
		if (!pw_update (&newpw)) {
			fprintf (stderr,
				 _("%s: line %d: can't update entry\n"),
				 Prog, line);
			errors++;
			continue;
		}
	}

	/*
	 * Any detected errors will cause the entire set of changes to be
	 * aborted. Unlocking the password file will cause all of the
	 * changes to be ignored. Otherwise the file is closed, causing the
	 * changes to be written out all at once, and then unlocked
	 * afterwards.
	 */
	if (errors) {
		fprintf (stderr,
			 _("%s: error detected, changes ignored\n"), Prog);
		(void) gr_unlock ();
		if (is_shadow)
			spw_unlock ();
		(void) pw_unlock ();
		exit (1);
	}
	if (!pw_close () || (is_shadow && !spw_close ()) || !gr_close ()) {
		fprintf (stderr, _("%s: error updating files\n"), Prog);
		(void) gr_unlock ();
		if (is_shadow)
			spw_unlock ();
		(void) pw_unlock ();
		exit (1);
	}

	nscd_flush_cache ("passwd");
	nscd_flush_cache ("group");

	(void) gr_unlock ();
	if (is_shadow)
		spw_unlock ();
	(void) pw_unlock ();

#ifdef USE_PAM
	if (retval == PAM_SUCCESS)
		pam_end (pamh, PAM_SUCCESS);
#endif				/* USE_PAM */

	exit (0);
	/* NOT REACHED */
}
