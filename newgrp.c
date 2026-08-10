/*
 * Copyright 1990, 1991, 1992, 1993, John F. Haugh II
 * All rights reserved.
 *
 * Permission is granted to copy and create derivative works for any
 * non-commercial purpose, provided this copyright notice is preserved
 * in all copies of source code, or included in human readable form
 * and conspicuously displayed on all copies of object code or
 * distribution media.
 *
 * This software is provided on an AS-IS basis and the author makes
 * no warrantee of any kind.
 */

#include <sys/types.h>
#ifndef	BSD
#include <string.h>
#else
#include <strings.h>
#endif
#include <stdio.h>
#include <grp.h>
#include "pwd.h"
#include <termio.h>
#ifdef SYS3
#include <sys/ioctl.h>
#endif
#include "config.h"

#if !defined(BSD) && !defined(SUN) && !defined(SUN4)
#define	bzero(p,n) memset(p, 0, n)
#endif

#ifndef	lint
static	char	sccsid[] = "@(#)newgrp.c	3.11	07:34:23	08 Apr 1993";
#endif

#ifdef	NGROUPS
int	ngroups;
gid_t	groups[NGROUPS];
#endif

char	*getpass();
char	*getenv();
char	*pw_encrypt();
struct	passwd	*pwd;
struct	passwd	*getpwuid();
struct	passwd	*getpwnam();

#ifdef	SHADOWPWD
#include "shadow.h"
struct	spwd	*spwd;
struct	spwd	*getspnam();
#endif
#ifdef	SHADOWGRP
struct	sgrp	*sgrp;
struct	sgrp	*getsgnam();
#endif

#ifdef	USE_SYSLOG
#include <syslog.h>

/*VARARGS*/ int syslog();

#ifndef	LOG_WARN
#define	LOG_WARN LOG_WARNING
#endif	/* !LOG_WARN */
#endif	/* USE_SYSLOG */

struct	group	*grp;
struct	group	*getgrgid();
struct	group	*getgrnam();

char	*getlogin();
char	*crypt();
char	*getpass();
char	*getenv();
char	*pw_encrypt();
void	shell();

char	*name;
char	*group;
gid_t	gid;
int	cflag;

char	*Prog;
char	prog[BUFSIZ];
char	base[BUFSIZ];
char	passwd[BUFSIZ];
char	*cpasswd;
char	*salt;

#ifndef	MAXENV
#define	MAXENV	64
#endif

char	*newenvp[MAXENV];
int	newenvc = 0;
int	maxenv = MAXENV;

/*
 * usage - print command usage message
 */

usage ()
{
	if (strcmp (Prog, "sg") != 0)
		fprintf (stderr, "usage: newgrp [ - ] [ group ]\n");
	else
		fprintf (stderr, "usage: sg group [ command ]\n");
}

/*
 * newgrp - change the invokers current real and effective group id
 */

main (argc, argv, envp)
int	argc;
char	**argv;
char	**envp;
{
	int	initflag = 0;
	int	needspasswd = 0;
	int	i;
	char	*cp;
	char	*command;

	/*
	 * save my name for error messages and save my real gid incase
	 * of errors.  if there is an error i have to exec a new login
	 * shell for the user since her old shell won't have fork'd to
	 * create the process.  skip over the program name to the next
	 * command line argument.
	 */

	if (Prog = strrchr (argv[0], '/'))
		Prog++;
	else
		Prog = argv[0];

#ifdef	USE_SYSLOG
	openlog (Prog, LOG_PID|LOG_CONS|LOG_NOWAIT, LOG_AUTH);
#endif

	gid = getgid ();
	argc--; argv++;

	/*
	 * here i get to determine my current name.  i do this to validate
	 * my access to the requested group.  the validation works like
	 * this -
	 *	1) get the name associated with my current user id
	 *	2) get my login name, as told by getlogin().
	 *	3) if they match, my name is the login name
	 *	4) if they don't match, my name is the name in the
	 *	   password file.
	 *
	 * this isn't perfect, but it works more often then not.  i have
	 * to do this here so i can get the login name to find the
	 * login group.
	 */

	pwd = getpwuid (getuid ());

	if (! (name = getlogin ()) || strcmp (name, pwd->pw_name) != 0)
		name = pwd->pw_name;

	if (! (pwd = getpwnam (name))) {
		fprintf (stderr, "unknown user: %s\n", name);
#ifdef	USE_SYSLOG
		syslog (LOG_WARN, "unknown user `%s', uid `%d'\n",
			name, getuid ());
		closelog ();
#endif
		goto failure;
	}

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
	 *	sg [ - ]
	 *	sg [ - ] groupid [ command ]
	 */

	if (argc > 0 && argv[0][0] == '-' && argv[0][1] == '\0') {
		argc--; argv++;
		initflag = 1;
	}
	if (strcmp (Prog, "newgrp") != 0) {

		/* 
		 * Do the command line for everything that is
		 * not "newgrp".
		 */

		if (argc > 0 && argv[0][0] != '-') {
			group = argv[0];
			argc--; argv++;
		} else {
			usage ();
#ifdef	USE_SYSLOG
			closelog ();
#endif
			exit (1);
		}
		if (argc > 0) {
			command = argv[1];
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
				fprintf (stderr, "unknown gid: %d\n",
					pwd->pw_gid);
#ifdef	USE_SYSLOG
				syslog (LOG_CRIT, "unknown gid: %d\n",
					pwd->pw_gid);
#endif
				goto failure;
			}
		}
	}
#ifdef	NGROUPS

	/*
	 * get the current users groupset.  the new group will be
	 * added to the concurrent groupset if there is room, otherwise
	 * you get a nasty message but at least your real and effective
	 * group id's are set.
	 */

	ngroups = getgroups (0, 0);
	if (ngroups > 0)
		getgroups (ngroups, groups);
#endif

	/*
	 * now we put her in the new group.  the password file entry for
	 * her current user id has been gotten.  if there was no optional
	 * group argument she will have her real and effective group id
	 * set to the value from her password file entry.  otherwise
	 * we validate her access to the specified group.
	 */

	if (group == (char *) 0) {
		if (! (grp = getgrgid (pwd->pw_gid))) {
			fprintf (stderr, "unknown gid: %d\n", pwd->pw_gid);
			goto failure;
		}
	} else if (! (grp = getgrnam (group))) {
		fprintf (stderr, "unknown group: %s\n", group);
		goto failure;
	}
#ifdef	SHADOWGRP
	sgrp = getsgnam (group);
#endif

	/*
	 * see if she is a member of this group.
	 */

	for (i = 0;grp->gr_mem[i];i++)
		if (strcmp (name, grp->gr_mem[i]) == 0)
			break;

	/*
	 * if she isn't a member, she needs to provide the
	 * group password.  if there is no group password, she
	 * will be denied access anyway.
	 */

	if (grp->gr_mem[i] == (char *) 0)
		needspasswd = 1;

#ifdef	SHADOWGRP
	if (sgrp) {

		/*
		 * Do the tests again with the shadow group entry.
		 */

		for (i = 0;sgrp->sg_mem[i];i++)
			if (strcmp (name, sgrp->sg_mem[i]) == 0)
				break;

		needspasswd = sgrp->sg_mem[i] == (char *) 0;
	}
#endif
#ifdef	SHADOWPWD

	/*
	 * if she does not have either a shadowed password,
	 * or a regular password, and the group has a password,
	 * she needs to give the group password.
	 */

	if (spwd = getspnam (name)) {
		if (spwd->sp_pwdp[0] == '\0' && grp->gr_passwd[0])
			needspasswd = 1;
#ifdef	SHADOWGRP
		if (spwd->sp_pwdp[0] == '\0' && sgrp != 0)
			needspasswd = sgrp->sg_passwd[0] != '\0';
#endif
	} else {
		if (pwd->pw_passwd[0] == '\0' && grp->gr_passwd[0])
			needspasswd = 1;
#ifdef	SHADOWGRP
		if (pwd->pw_passwd[0] == '\0' && sgrp != 0)
			needspasswd = sgrp->sg_passwd[0] != '\0';
#endif
	}
#else

	/*
	 * if she does not have a regular password she will have
	 * to give the group password, if one exists.
	 */

	if (pwd->pw_passwd[0] == '\0' && grp->gr_passwd[0])
		needspasswd = 1;
#ifdef	SHADOWGRP
	if (pwd->pw_passwd[0] == '\0' && sgrp != 0)
		needspasswd = sgrp->sg_passwd[0] != '\0';
#endif
#endif

	/*
	 * now i see about letting her into the group she requested.
	 * if she is the root user, i'll let her in without having to
	 * prompt for the password.  otherwise i ask for a password
	 * if she flunked one of the tests above.  note that she
	 * won't have to provide the password to her login group even
	 * if she isn't listed as a member.
	 */

	if (getuid () != 0 && needspasswd) {
		char	*encrypted;

		encrypted = grp->gr_passwd;
#ifdef	SHADOWGRP
		if (sgrp)
			encrypted = sgrp->sg_passwd;
#endif
		passwd[0] = '\0';

		if (encrypted[0]) {

		/*
		 * get the password from her, and set the salt for
		 * the decryption from the group file.
		 */

			if (! (cp = getpass ("Password:")))
				goto failure;

			strcpy (passwd, cp);
			bzero (cp, strlen (cp));
			salt = encrypted;
		} else {

		/*
		 * there is no password, print out "Sorry" and give up
		 */

			fputs ("Sorry\n", stderr);
			goto failure;
		}

		/*
		 * encrypt the key she gave us using the salt from
		 * the password in the group file.  the result of
		 * this encryption must match the previously
		 * encrypted value in the file.
		 */

		cpasswd = pw_encrypt (passwd, salt);
		bzero (passwd, sizeof passwd);

		if (strcmp (cpasswd, encrypted) != 0) {
			fputs ("Sorry\n", stderr);
#ifdef	USE_SYSLOG
		syslog (LOG_INFO, "Invalid password for `%s' from `%s'\n",
			group, name);
#endif
			goto failure;
		}
	}

	/*
	 * all successful validations pass through this point.  the
	 * group id will be set, and the group added to the concurrent
	 * groupset.
	 */

#ifdef	USE_SYSLOG
	if (getdef_bool ("SYSLOG_SU_ENAB"))
		syslog (LOG_INFO, "user `%s' switched to group `%s'\n", name, group);
#endif
	gid = grp->gr_gid;
#ifdef	NGROUPS

	/*
	 * i am going to try to add her new group id to her concurrent
	 * group set.  if the group id is already present i'll just
	 * skip this part.  if the group doesn't fit, i'll complain
	 * loudly and skip this part ...
	 */

	for (i = 0;i < ngroups;i++) {
		if (gid == groups[i])
			break;
	}
	if (i == ngroups) {
		if (ngroups == NGROUPS) {
			fprintf (stderr, "too many groups\n");
		} else {
			groups[ngroups++] = gid;
			if (setgroups (ngroups, groups)) {
				fprintf (stderr, "%s: ", Prog);
				perror ("unable to set groups");
			}
		}
	}
#endif

okay:

	/*
	 * i set her group id either to the value she requested, or
	 * to the original value if the newgrp failed.
	 */

	if (setgid (gid))
		perror ("setgid");

	if (setuid (getuid ()))
		perror ("setuid");

	/*
	 * see if the "-c" flag was used.  if it was, i just create a
	 * shell command for her using the argument that followed the
	 * "-c" flag.
	 */

	if (cflag) {
		execl ("/bin/sh", "sh", "-c", command, (char *) 0);
		perror ("/bin/sh");
#ifdef	USE_SYSLOG
		closelog ();
#endif
		exit (255);
	}

	/*
	 * i have to get the pathname of her login shell.  as a favor,
	 * i'll try her environment for a $SHELL value first, and
	 * then try the password file entry.  obviously this shouldn't
	 * be in the restricted command directory since it could be
	 * used to leave the restricted environment.
	 */

	if (! initflag && (cp = getenv ("SHELL")))
		strncpy (prog, cp, sizeof prog);
	else if (pwd->pw_shell && pwd->pw_shell[0])
		strncpy (prog, pwd->pw_shell, sizeof prog);
	else
		strcpy (prog, "/bin/sh");

	/*
	 * now i try to find the basename of the login shell.  this
	 * will become argv[0] of the spawned command.
	 */

	if (cp = strrchr (prog, '/'))
		cp++;
	else
		cp = prog;

	/*
	 * to have the shell perform login processing i will set the
	 * first character in the first argument to a "-".
	 */

	if (initflag)
		strcat (strcpy (base, "-"), cp);
	else
		strcpy (base, cp);

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
		chdir (pwd->pw_dir);
		while (*envp) {
			if (strncmp (*envp, "PATH=", 5) == 0 ||
					strncmp (*envp, "HOME=", 5) == 0 ||
					strncmp (*envp, "SHELL=", 6) == 0 ||
					strncmp (*envp, "TERM=", 5) == 0)
				addenv (*envp);

			envp++;
		}
	} else {
		while (*envp)
			addenv (*envp++);
	}

	/*
	 * exec the login shell and go away.  we are trying to get
	 * back to the previous environment which should be the
	 * user's login shell.
	 */

	shell (prog, base);
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

	if (strcmp (Prog, "newgrp") != 0) {
#ifdef	USE_SYSLOG
		closelog ();
#endif
		exit (1);
	}
	
	/*
	 * The GID is still set to the old value, so now I can
	 * give the user back her shell.
	 */

	goto okay;
}
