/*
 * Copyright 1990 - 1994, Julianne Frances Haugh
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
RCSID(PKG_VER "$Id: newgrp.c,v 1.16 2000/09/02 18:40:44 marekm Exp $")

#include <stdio.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>

#include "prototypes.h"
#include "defines.h"

#include "getdef.h"

extern char **environ;

#ifdef HAVE_SETGROUPS
static int ngroups;
static GETGROUPS_T *grouplist;
#endif

static char *Prog;
static int is_newgrp;

/* local function prototypes */
static void usage(void);

/*
 * usage - print command usage message
 */

static void
usage(void)
{
	if (is_newgrp)
		fprintf (stderr, _("usage: newgrp [ - ] [ group ]\n"));
	else
		fprintf (stderr, _("usage: sg group [[-c] command ]\n"));
}

/*
 * newgrp - change the invokers current real and effective group id
 */

int
main(int argc, char **argv)
{
	int	initflag = 0;
	int	needspasswd = 0;
	int	i;
	int their_grp = 0;
	int cflag = 0;
	gid_t gid;
	char *cp;
	const char *cpasswd, *name, *prog;
	char *group = NULL;
	char	*command=NULL;
	char	**envp = environ;
	struct passwd *pwd;
	struct group *grp;
#ifdef SHADOWPWD
	struct spwd *spwd;
#endif
#ifdef SHADOWGRP
	struct sgrp *sgrp;
#endif

#if ENABLE_NLS
	/* XXX - remove when gettext is safe to use in setuid programs */
	sanitize_env();
#endif

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
                  
	/*
	 * save my name for error messages and save my real gid incase
	 * of errors.  if there is an error i have to exec a new login
	 * shell for the user since her old shell won't have fork'd to
	 * create the process.  skip over the program name to the next
	 * command line argument.
	 */

	Prog = Basename(argv[0]);
	is_newgrp = (strcmp(Prog, "newgrp") == 0);
	OPENLOG(is_newgrp ? "newgrp" : "sg");
	gid = getgid();
	argc--; argv++;

	initenv();

	pwd = get_my_pwent();
	if (!pwd) {
		fprintf (stderr, _("unknown uid: %d\n"), (int) getuid());
		SYSLOG((LOG_WARN, "unknown uid %d\n", (int) getuid()));
		closelog();
		exit(1);
	}
	name = pwd->pw_name;

	/*
	 * Parse the command line.  There are two accepted flags.  The
	 * first is "-", which for newgrp means to re-create the entire
	 * environment as though a login had been performed, and "-c",
	 * which for sg causes a command string to be executed.
	 *
	 * The next argument, if present, must be the new group name.
	 * Any remaining remaining arguments will be used to execute a
	 * command as the named group.  If the group name isn't present,
	 * I just use the login group ID of the current user.
	 *
	 * The valid syntax are
	 *	newgrp [ - ] [ groupid ]
	 *	newgrp [ -l ] [ groupid ]
	 *	sg [ - ]
	 *	sg [ - ] groupid [ command ]
	 */

	if (argc > 0 && (!strcmp(argv[0], "-") || !strcmp(argv[0], "-l"))) {
		argc--; argv++;
		initflag = 1;
	}
	if (!is_newgrp) {
		/* 
		 * Do the command line for everything that is
		 * not "newgrp".
		 */

		if (argc > 0 && argv[0][0] != '-') {
			group = argv[0];
			argc--; argv++;
		} else {
			usage ();
			closelog();
			exit (1);
		}
		if (argc > 0) {

			/* skip -c if specified so both forms work:
			   "sg group -c command" (as in the man page) or
			   "sg group command" (as in the usage message).  */

			if (argc > 1 && strcmp(argv[0], "-c") == 0)
				command = argv[1];
			else
				command = argv[0];
			cflag++;
		}
	} else {

		/*
		 * Do the command line for "newgrp".  It's just
		 * making sure there aren't any flags and getting
		 * the new group name.
		 */

		if (argc > 0 && argv[0][0] == '-') {
			usage ();
			goto failure;
		} else if (argv[0] != (char *) 0) {
			group = argv[0];
		} else {

			/*
			 * get the group file entry for her login group id.
			 * the entry must exist, simply to be annoying.
			 */

			if (! (grp = getgrgid (pwd->pw_gid))) {
				fprintf(stderr, _("unknown gid: %ld\n"),
					(long) pwd->pw_gid);
				SYSLOG((LOG_CRIT, "unknown gid: %ld\n",
					(long) pwd->pw_gid));
				goto failure;
			}
		}
	}

#ifdef HAVE_SETGROUPS
	/*
	 * get the current users groupset.  the new group will be
	 * added to the concurrent groupset if there is room, otherwise
	 * you get a nasty message but at least your real and effective
	 * group id's are set.
	 */

	/* don't use getgroups(0, 0) - it doesn't work on some systems */
	i = 16;
	for (;;) {
		grouplist = (GETGROUPS_T *) xmalloc(i * sizeof(GETGROUPS_T));
		ngroups = getgroups(i, grouplist);
		if (i > ngroups && !(ngroups == -1 && errno == EINVAL))
			break;
		/* not enough room, so try allocating a larger buffer */
		free(grouplist);
		i *= 2;
	}
	if (ngroups < 0) {
		perror("getgroups");
		exit(1);
	}
#endif /* HAVE_SETGROUPS */

	/*
	 * now we put her in the new group.  the password file entry for
	 * her current user id has been gotten.  if there was no optional
	 * group argument she will have her real and effective group id
	 * set to the value from her password file entry.  otherwise
	 * we validate her access to the specified group.
	 */

	if (group == (char *) 0) {
		if (! (grp = getgrgid (pwd->pw_gid))) {
			fprintf (stderr, _("unknown gid: %d\n"), pwd->pw_gid);
			goto failure;
		}
		group = grp->gr_name;
		their_grp = 1;
	} else if (! (grp = getgrnam (group))) {
		fprintf (stderr, _("unknown group: %s\n"), group);
		goto failure;
	}
#ifdef SHADOWGRP
	if ((sgrp = getsgnam (group))) {
		grp->gr_passwd = sgrp->sg_passwd;
		grp->gr_mem = sgrp->sg_mem;
	}
#endif

	/*
	 * see if she is a member of this group.
	 * if she isn't a member, she needs to provide the
	 * group password.  if there is no group password, she
	 * will be denied access anyway.
	 *
	 * we also check if this is the users default group, eg.
	 * they aren't a member, but this is the group listed as
	 * the one they belong to in their pwd entry.
	 */

	if (!is_on_list(grp->gr_mem, name) && !their_grp)
		needspasswd = 1;

	/*
	 * if she does not have either a shadowed password,
	 * or a regular password, and the group has a password,
	 * she needs to give the group password.
	 */

#ifdef	SHADOWPWD
	if ((spwd = getspnam (name)))
		pwd->pw_passwd = spwd->sp_pwdp;
#endif

	if (pwd->pw_passwd[0] == '\0' && grp->gr_passwd[0])
		needspasswd = 1;

	/*
	 * now i see about letting her into the group she requested.
	 * if she is the root user, i'll let her in without having to
	 * prompt for the password.  otherwise i ask for a password
	 * if she flunked one of the tests above.  note that she
	 * won't have to provide the password to her login group even
	 * if she isn't listed as a member.
	 */

	if (getuid () != 0 && needspasswd) {

		/*
		 * get the password from her, and set the salt for
		 * the decryption from the group file.
		 */

		if (! (cp = getpass (_("Password: "))))
			goto failure;

		/*
		 * encrypt the key she gave us using the salt from
		 * the password in the group file.  the result of
		 * this encryption must match the previously
		 * encrypted value in the file.
		 */

		cpasswd = pw_encrypt (cp, grp->gr_passwd);
		strzero(cp);

		if (grp->gr_passwd[0] == '\0') {
		/*
		 * there is no password, print out "Sorry" and give up
		 */
			sleep(1);
			fputs (_("Sorry.\n"), stderr);
			goto failure;
		}

		if (strcmp (cpasswd, grp->gr_passwd) != 0) {
			SYSLOG((LOG_INFO,
				"Invalid password for group `%s' from `%s'\n",
				group, name));
			sleep(1);
			fputs (_("Sorry.\n"), stderr);
			goto failure;
		}
	}

	/*
	 * all successful validations pass through this point.  the
	 * group id will be set, and the group added to the concurrent
	 * groupset.
	 */

#ifdef	USE_SYSLOG
	if (getdef_bool ("SYSLOG_SG_ENAB"))
		SYSLOG((LOG_INFO, "user `%s' switched to group `%s'\n",
			name, group));
#endif
	gid = grp->gr_gid;

#ifdef HAVE_SETGROUPS
	/*
	 * i am going to try to add her new group id to her concurrent
	 * group set.  if the group id is already present i'll just
	 * skip this part.  if the group doesn't fit, i'll complain
	 * loudly and skip this part ...
	 */

	for (i = 0;i < ngroups;i++) {
		if (gid == grouplist[i])
			break;
	}
	if (i == ngroups) {
		if (ngroups >= NGROUPS_MAX) {
			fprintf (stderr, _("too many groups\n"));
		} else {
			grouplist[ngroups++] = gid;
			if (setgroups(ngroups, grouplist)) {
				perror("setgroups");
			}
		}
	}
#endif

okay:
	/*
	 * i set her group id either to the value she requested, or
	 * to the original value if the newgrp failed.
	 */

	if (setgid(gid))
		perror("setgid");

	if (setuid(getuid())) {
		perror("setuid");
		exit(1);
	}

	/*
	 * see if the "-c" flag was used.  if it was, i just create a
	 * shell command for her using the argument that followed the
	 * "-c" flag.
	 */

	if (cflag) {
		closelog();
		execl("/bin/sh", "sh", "-c", command, (char *) 0);
		if (errno == ENOENT) {
			perror("/bin/sh");
			exit(127);
		} else {
			perror("/bin/sh");
			exit(126);
		}
	}

	/*
	 * i have to get the pathname of her login shell.  as a favor,
	 * i'll try her environment for a $SHELL value first, and
	 * then try the password file entry.  obviously this shouldn't
	 * be in the restricted command directory since it could be
	 * used to leave the restricted environment.
	 */

	if (! initflag && (cp = getenv ("SHELL")))
		prog = cp;
	else if (pwd->pw_shell && pwd->pw_shell[0])
		prog = pwd->pw_shell;
	else
		prog = "/bin/sh";

	/*
	 * now i try to find the basename of the login shell.  this
	 * will become argv[0] of the spawned command.
	 */

	cp = Basename((char *) prog);

#ifdef	SHADOWPWD
	endspent ();
#endif
#ifdef	SHADOWGRP
	endsgent ();
#endif
	endpwent ();
	endgrent ();

	/*
	 * switch back to her home directory if i am doing login
	 * initialization.
	 */

	if (initflag) {
		if (chdir (pwd->pw_dir))
			perror("chdir");

		while (*envp) {
			if (strncmp (*envp, "PATH=", 5) == 0 ||
					strncmp (*envp, "HOME=", 5) == 0 ||
					strncmp (*envp, "SHELL=", 6) == 0 ||
					strncmp (*envp, "TERM=", 5) == 0)
				addenv(*envp, NULL);

			envp++;
		}
	} else {
		while (*envp)
			addenv(*envp++, NULL);
	}

	/* sanitize_env() removes $HOME from the environment (maybe it
	   shouldn't - please tell me if you are sure that $HOME can't
	   cause security problems) - add it from user's passwd entry.
	*/
	addenv("HOME", pwd->pw_dir);

	/*
	 * exec the login shell and go away.  we are trying to get
	 * back to the previous environment which should be the
	 * user's login shell.
	 */

	shell(prog, initflag ? (char *)0 : cp);
	/*NOTREACHED*/

failure:
	/*
	 * this is where all failures land.  the group id will not
	 * have been set, so the setgid() below will set me to the
	 * original group id i had when i was invoked.
	 */

	/*
	 * only newgrp needs to re-exec the user's shell.  that is
	 * because the shell doesn't recognize "sg", so it doesn't
	 * "exec" this command.
	 */

	if (!is_newgrp) {
		closelog();
		exit (1);
	}
	
	/*
	 * The GID is still set to the old value, so now I can
	 * give the user back her shell.
	 */

	goto okay;
}
