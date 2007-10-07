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
 * ARE DISCLAIMED.  IN NO EVENT SHALL JULIE HAUGH OR CONTRIBUTORS BE LIABLE
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

#include "rcsid.h"
RCSID(PKG_VER "$Id: newusers.c,v 1.11 2000/08/26 18:27:18 marekm Exp $")

#include <sys/types.h>
#include <sys/stat.h>
#include "prototypes.h"
#include "defines.h"
#include <stdio.h>
#include <pwd.h>
#include <grp.h>
#include <fcntl.h>

static char *Prog;

#include "getdef.h"
#include "pwio.h"
#include "groupio.h"

#ifdef	SHADOWPWD
#include "shadowio.h"

static int is_shadow;
#endif

/* local function prototypes */
static void usage(void);
static int add_group(const char *, const char *, gid_t *);
static int add_user(const char *, const char *, uid_t *, gid_t);
static void update_passwd(struct passwd *, const char *);
static int add_passwd(struct passwd *, const char *);

/*
 * usage - display usage message and exit
 */

static void
usage(void)
{
	fprintf(stderr, _("Usage: %s [ input ]\n"), Prog);
	exit(1);
}

/*
 * add_group - create a new group or add a user to an existing group
 */

static int
add_group(const char *name, const char *gid, gid_t *ngid)
{
	const struct passwd *pwd;
	const struct group *grp;
	struct	group	grent;
	char	*members[2];
	int	i;

	/*
	 * Start by seeing if the named group already exists.  This
	 * will be very easy to deal with if it does.
	 */

	if ((grp = gr_locate (gid))) {
add_member:
		grent = *grp;
		*ngid = grent.gr_gid;
		for (i = 0;grent.gr_mem[i] != (char *) 0;i++)
			if (strcmp (grent.gr_mem[i], name) == 0)
				return 0;

		grent.gr_mem = (char **) xmalloc (sizeof (char *) * (i + 2));
		memcpy (grent.gr_mem, grp->gr_mem, sizeof (char *) * (i + 2));
		grent.gr_mem[i] = xstrdup (name);
		grent.gr_mem[i + 1] = (char *) 0;

		return ! gr_update (&grent);
	}

	/*
	 * The group did not exist, so I try to figure out what the
	 * GID is going to be.  The gid parameter is probably "", meaning
	 * I figure out the GID from the password file.  I want the UID
	 * and GID to match, unless the GID is already used.
	 */

	if (gid[0] == '\0') {
		i = 100;
		for (pw_rewind ();(pwd = pw_next ());) {
			if (pwd->pw_uid >= i)
				i = pwd->pw_uid + 1;
		}
		for (gr_rewind ();(grp = gr_next ());) {
			if (grp->gr_gid == i) {
				i = -1;
				break;
			}
		}
	} else if (gid[0] >= '0' && gid[0] <= '9') {

	/*
	 * The GID is a number, which means either this is a brand new
	 * group, or an existing group.  For existing groups I just add
	 * myself as a member, just like I did earlier.
	 */

		i = atoi (gid);
		for (gr_rewind ();(grp = gr_next ());)
			if (grp->gr_gid == i)
				goto add_member;
	} else

	/*
	 * The last alternative is that the GID is a name which is not
	 * already the name of an existing group, and I need to figure
	 * out what group ID that group name is going to have.
	 */

		i = -1;

	/*
	 * If I don't have a group ID by now, I'll go get the
	 * next one.
	 */

	if (i == -1) {
		for (i = 100, gr_rewind ();(grp = gr_next ());)
			if (grp->gr_gid >= i)
				i = grp->gr_gid + 1;
	}

	/*
	 * Now I have all of the fields required to create the new
	 * group.
	 */

	if (gid[0] && (gid[0] <= '0' || gid[0] >= '9'))
		grent.gr_name = xstrdup(gid);
	else
		grent.gr_name = xstrdup(name);

	grent.gr_passwd = "x";  /* XXX warning: const */
	grent.gr_gid = i;
	members[0] = xstrdup(name);
	members[1] = (char *) 0;
	grent.gr_mem = members;

	*ngid = grent.gr_gid;
	return ! gr_update (&grent);
}

/*
 * add_user - create a new user ID
 */

static int
add_user(const char *name, const char *uid, uid_t *nuid, gid_t gid)
{
	const struct passwd *pwd;
	struct	passwd	pwent;
	uid_t	i;

	/*
	 * The first guess for the UID is either the numerical UID
	 * that the caller provided, or the next available UID.
	 */

	if (uid[0] >= '0' && uid[0] <= '9') {
		i = atoi (uid);
	} else if (uid[0] && (pwd = pw_locate (uid))) {
		i = pwd->pw_uid;
	} else {
		i = 100;
		for (pw_rewind ();(pwd = pw_next ());)
			if (pwd->pw_uid >= i)
				i = pwd->pw_uid + 1;
	}

	/*
	 * I don't want to fill in the entire password structure
	 * members JUST YET, since there is still more data to be
	 * added.  So, I fill in the parts that I have.
	 */

	pwent.pw_name = xstrdup(name);
	pwent.pw_passwd = "x";  /* XXX warning: const */
#ifdef	ATT_AGE
	pwent.pw_age = "";
#endif
#ifdef	ATT_COMMENT
	pwent.pw_comment = "";
#endif
#ifdef	BSD_QUOTA
	pwent.pw_quota = 0;
#endif
	pwent.pw_uid = i;
	pwent.pw_gid = gid;
	pwent.pw_gecos = "";  /* XXX warning: const */
	pwent.pw_dir = "";  /* XXX warning: const */
	pwent.pw_shell = "";  /* XXX warning: const */

	*nuid = i;
	return ! pw_update (&pwent);
}

static void
update_passwd(struct passwd *pwd, const char *passwd)
{
	pwd->pw_passwd = pw_encrypt(passwd, crypt_make_salt());
#ifdef ATT_AGE
	if (strlen(pwd->pw_age) == 4) {
		static char newage[5];
		extern char *l64a();

		strcpy(newage, pwd->pw_age);
		strcpy(newage + 2, l64a(time((time_t *) 0) / WEEK));
		pwd->pw_age = newage;
	}
#endif
}

/*
 * add_passwd - add or update the encrypted password
 */

static int
add_passwd(struct passwd *pwd, const char *passwd)
{
#ifdef	SHADOWPWD
	const struct spwd *sp;
	struct	spwd	spent;
#endif

	/*
	 * In the case of regular password files, this is real
	 * easy - pwd points to the entry in the password file.
	 * Shadow files are harder since there are zillions of
	 * things to do ...
	 */

	if (!is_shadow) {
		update_passwd(pwd, passwd);
		return 0;
	}

#ifdef SHADOWPWD
	/*
	 * Do the first and easiest shadow file case.  The user
	 * already exists in the shadow password file.
	 */

	if ((sp = spw_locate (pwd->pw_name))) {
		spent = *sp;
		spent.sp_pwdp = pw_encrypt(passwd, crypt_make_salt());
		return ! spw_update (&spent);
	}

	/*
	 * Pick the next easiest case - the user has an encrypted
	 * password which isn't equal to "x".  The password was set
	 * to "x" earlier when the entry was created, so this user
	 * would have to have had the password set someplace else.
	 */

	if (strcmp (pwd->pw_passwd, "x") != 0) {
		update_passwd(pwd, passwd);
		return 0;
	}

	/*
	 * Now the really hard case - I need to create an entirely
	 * new shadow password file entry.
	 */

	spent.sp_namp = pwd->pw_name;
	spent.sp_pwdp = pw_encrypt(passwd, crypt_make_salt());
	spent.sp_lstchg = time((time_t *) 0) / SCALE;
	spent.sp_min = getdef_num("PASS_MIN_DAYS", 0);
					/* 10000 is infinity this week */
	spent.sp_max = getdef_num("PASS_MAX_DAYS", 10000);
	spent.sp_warn = getdef_num("PASS_WARN_AGE", -1);
	spent.sp_inact = -1;
	spent.sp_expire = -1;
	spent.sp_flag = -1;

	return ! spw_update (&spent);
#endif
}

int
main(int argc, char **argv)
{
	char	buf[BUFSIZ];
	char	*fields[8];
	int	nfields;
	char	*cp;
	const struct passwd *pw;
	struct	passwd	newpw;
	int	errors = 0;
	int	line = 0;
	uid_t	uid;
	gid_t	gid;

	Prog = Basename(argv[0]);

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
                   
	if (argc > 1 && argv[1][0] == '-')
		usage ();

	if (argc == 2) {
		if (! freopen (argv[1], "r", stdin)) {
			snprintf(buf, sizeof buf, "%s: %s", Prog, argv[1]);
			perror (buf);
			exit (1);
		}
	}

	/*
	 * Lock the password files and open them for update.  This will
	 * bring all of the entries into memory where they may be
	 * searched for an modified, or new entries added.  The password
	 * file is the key - if it gets locked, assume the others can
	 * be locked right away.
	 */

	if (!pw_lock()) {
		fprintf (stderr, _("%s: can't lock /etc/passwd.\n"), Prog);
		exit (1);
	}
#ifdef	SHADOWPWD
	is_shadow = spw_file_present();

	if ((is_shadow && !spw_lock()) || !gr_lock())
#else
	if (!gr_lock())
#endif
	{
		fprintf (stderr, _("%s: can't lock files, try again later\n"),
			Prog);
		(void) pw_unlock ();
#ifdef	SHADOWPWD
		if (is_shadow)
			spw_unlock();
#endif
		exit (1);
	}
#ifdef	SHADOWPWD
	if (!pw_open(O_RDWR) || (is_shadow && !spw_open(O_RDWR)) || !gr_open(O_RDWR))
#else
	if (!pw_open(O_RDWR) || !gr_open(O_RDWR))
#endif
	{
		fprintf (stderr, _("%s: can't open files\n"), Prog);
		(void) pw_unlock ();
#ifdef	SHADOWPWD
		if (is_shadow)
			spw_unlock();
#endif
		(void) gr_unlock ();
		exit (1);
	}

	/*
	 * Read each line.  The line has the same format as a password
	 * file entry, except that certain fields are not contrained to
	 * be numerical values.  If a group ID is entered which does
	 * not already exist, an attempt is made to allocate the same
	 * group ID as the numerical user ID.  Should that fail, the
	 * next available group ID over 100 is allocated.  The pw_gid
	 * field will be updated with that value.
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
		 * Break the string into fields and screw around with
		 * them.  There MUST be 7 colon separated fields,
		 * although the values aren't that particular.
		 */

		for (cp = buf, nfields = 0;nfields < 7;nfields++) {
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
		 * Now the fields are processed one by one.  The first
		 * field to be processed is the group name.  A new
		 * group will be created if the group name is non-numeric
		 * and does not already exist.  The named user will be
		 * the only member.  If there is no named group to be a
		 * member of, the UID will be figured out and that value
		 * will be a candidate for a new group, if that group ID
		 * exists, a whole new group ID will be made up.
		 */
		
		if (! (pw = pw_locate (fields[0])) &&
			add_group (fields[0], fields[3], &gid)) {
			fprintf (stderr, _("%s: line %d: can't create GID\n"),
				Prog, line);
			errors++;
			continue;
		}

		/*
		 * Now we work on the user ID.  It has to be specified
		 * either as a numerical value, or left blank.  If it
		 * is a numerical value, that value will be used, otherwise
		 * the next available user ID is computed and used.  After
		 * this there will at least be a (struct passwd) for the
		 * user.
		 */

		if (! pw && add_user (fields[0], fields[2], &uid, gid)) {
			fprintf (stderr, _("%s: line %d: can't create UID\n"),
				Prog, line);
			errors++;
			continue;
		}

		/*
		 * The password, gecos field, directory, and shell fields
		 * all come next.
		 */

		if (! (pw = pw_locate (fields[0]))) {
			fprintf (stderr, _("%s: line %d: cannot find user %s\n"),
				Prog, line, fields[0]);
			errors++;
			continue;
		}
		newpw = *pw;

		if (add_passwd (&newpw, fields[1])) {
			fprintf (stderr, _("%s: line %d: can't update password\n"),
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

		if (newpw.pw_dir[0] && access(newpw.pw_dir, F_OK)) {
			if (mkdir (newpw.pw_dir,
					0777 & ~getdef_num("UMASK", 077)))
				fprintf (stderr, _("%s: line %d: mkdir failed\n"),
					Prog, line);
			else if (chown (newpw.pw_dir,
					newpw.pw_uid, newpw.pw_gid))
				fprintf (stderr, _("%s: line %d: chown failed\n"),
					Prog, line);
		}

		/*
		 * Update the password entry with the new changes made.
		 */

		if (! pw_update (&newpw)) {
			fprintf (stderr, _("%s: line %d: can't update entry\n"),
				Prog, line);
			errors++;
			continue;
		}
	}

	/*
	 * Any detected errors will cause the entire set of changes
	 * to be aborted.  Unlocking the password file will cause
	 * all of the changes to be ignored.  Otherwise the file is
	 * closed, causing the changes to be written out all at
	 * once, and then unlocked afterwards.
	 */

	if (errors) {
		fprintf (stderr, _("%s: error detected, changes ignored\n"), Prog);
		(void) gr_unlock ();
#ifdef	SHADOWPWD
		if (is_shadow)
			spw_unlock();
#endif
		(void) pw_unlock ();
		exit (1);
	}
#ifdef	SHADOWPWD
	if (!pw_close() || (is_shadow && !spw_close()) || !gr_close())
#else
	if (!pw_close() || ! gr_close())
#endif
	{
		fprintf (stderr, _("%s: error updating files\n"), Prog);
		(void) gr_unlock ();
#ifdef	SHADOWPWD
		if (is_shadow)
			spw_unlock();
#endif
		(void) pw_unlock ();
		exit (1);
	}
	(void) gr_unlock ();
#ifdef	SHADOWPWD
	if (is_shadow)
		spw_unlock();
#endif
	(void) pw_unlock ();

	exit (0);
	/*NOTREACHED*/
}
