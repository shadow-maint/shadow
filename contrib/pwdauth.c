/*
 * pwdauth.c - program to verify a given username/password pair.
 *
 * Run it with username in argv[1] (may be omitted - default is the
 * current user), and send it the password over a pipe on stdin.
 * Exit status: 0 - correct password, 1 - wrong password, >1 - other
 * errors.  For use with shadow passwords, this program should be
 * installed setuid root.
 *
 * This can be used, for example, by xlock - you don't have to install
 * this large and complex (== possibly insecure) program setuid root,
 * just modify it to run this simple program to do the authentication.
 *
 * Recent versions (xlockmore-3.9) are cleaner, and drop privileges as
 * soon as possible after getting the user's encrypted password.
 * Using this program probably doesn't make it more secure, and has one
 * disadvantage: since we don't get the encrypted user's password at
 * startup (but at the time the user is authenticated), it is not clear
 * how we should handle errors (like getpwnam() returning NULL).
 * - fail the authentication?  Problem: no way to unlock (other than kill
 *   the process from somewhere else) if the NIS server stops responding.
 * - succeed and unlock?  Problem: it's too easy to unlock by unplugging
 *   the box from the network and waiting until NIS times out...
 *
 * This program is Copyright (C) 1996 Marek Michalkiewicz
 * <marekm@i17linuxb.ists.pwr.wroc.pl>.
 *
 * It may be used and distributed freely for any purposes.  There is no
 * warranty - use at your own risk.  I am not liable for any damages etc.
 * If you improve it, please send me your changes.
 */

static char rcsid[] = "$Id$";

/*
 * Define USE_SYSLOG to use syslog() to log successful and failed
 * authentication.  This should be safe even if your system has
 * the infamous syslog buffer overrun security problem...
 */
#define USE_SYSLOG

/*
 * Define HAVE_GETSPNAM to get shadow passwords using getspnam().
 * Some systems don't have getspnam(), but getpwnam() returns
 * encrypted passwords only if running as root.
 *
 * According to the xlock source (not tested, except Linux) -
 * define: Linux, Solaris 2.x, SVR4, ...
 * undef: HP-UX with Secured Passwords, FreeBSD, NetBSD, QNX.
 * Known not supported (yet): Ultrix, OSF/1, SCO.
 */
#define HAVE_GETSPNAM

/*
 * Define HAVE_PW_ENCRYPT to use pw_encrypt() instead of crypt().
 * pw_encrypt() is like the standard crypt(), except that it may
 * support better password hashing algorithms.
 *
 * Define if linking with libshadow.a from the shadow password
 * suite (Linux, SunOS 4.x?).
 */
#undef HAVE_PW_ENCRYPT

/*
 * Define HAVE_AUTH_METHODS to support the shadow suite specific
 * extension: the encrypted password field contains a list of
 * administrator defined authentication methods, separated by
 * semicolons.  This program only supports the standard password
 * authentication method (a string that doesn't start with '@').
 */
#undef HAVE_AUTH_METHODS

/*
 * FAIL_DELAY - number of seconds to sleep before exiting if the
 * password was wrong, to slow down password guessing attempts.
 */
#define FAIL_DELAY 2

/* No user-serviceable parts below :-).  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pwd.h>

#ifdef USE_SYSLOG
#include <syslog.h>
#ifndef LOG_AUTHPRIV
#define LOG_AUTHPRIV LOG_AUTH
#endif
#endif

#ifdef HAVE_GETSPNAM
#include <shadow.h>
#endif

#ifdef HAVE_PW_ENCRYPT
extern char *pw_encrypt();
#define crypt pw_encrypt
#endif

/*
 * Read the password (one line) from fp.  We don't turn off echo
 * because we expect input from a pipe.
 */
static char *
get_line(fp)
	FILE *fp;
{
	static char buf[128];
	char *cp;
	int ch;

	cp = buf;
	while ((ch = getc(fp)) != EOF && ch != '\0' && ch != '\n') {
		if (cp >= buf + sizeof buf - 1)
			break;
		*cp++ = ch;
	}
	*cp = '\0';
	return buf;
}

/*
 * Get the password file entry for the current user.  If the name
 * returned by getlogin() is correct (matches the current real uid),
 * return the entry for that user.  Otherwise, return the entry (if
 * any) matching the current real uid.  Return NULL on failure.
 */
static struct passwd *
get_my_pwent()
{
	uid_t uid = getuid();
	char *name = getlogin();

	if (name && *name) {
		struct passwd *pw = getpwnam(name);

		if (pw && pw->pw_uid == uid)
			return pw;
	}
	return getpwuid(uid);
}

/*
 * Verify the password.  The system-dependent shadow support is here.
 */
static int
password_auth_ok(pw, pass)
	const struct passwd *pw;
	const char *pass;
{
	int result;
	char *cp;
#ifdef HAVE_AUTH_METHODS
	char *buf;
#endif
#ifdef HAVE_GETSPNAM
	struct spwd *sp;
#endif

	if (pw) {
#ifdef HAVE_GETSPNAM
		sp = getspnam(pw->pw_name);
		if (sp)
			cp = sp->sp_pwdp;
		else
#endif
			cp = pw->pw_passwd;
	} else
		cp = "xx";

#ifdef HAVE_AUTH_METHODS
	buf = strdup(cp);  /* will be modified by strtok() */
	if (!buf) {
		fprintf(stderr, "Out of memory.\n");
		exit(13);
	}
	cp = strtok(buf, ";");
	while (cp && *cp == '@')
		cp = strtok(NULL, ";");

	/* fail if no password authentication for this user */
	if (!cp)
		cp = "xx";
#endif

	if (*pass || *cp)
		result = (strcmp(crypt(pass, cp), cp) == 0);
	else
		result = 1;  /* user with no password */

#ifdef HAVE_AUTH_METHODS
	free(buf);
#endif
	return result;
}

/*
 * Main program.
 */
int
main(argc, argv)
	int argc;
	char **argv;
{
	struct passwd *pw;
	char *pass, *name;
	char myname[32];

#ifdef USE_SYSLOG
	openlog("pwdauth", LOG_PID | LOG_CONS, LOG_AUTHPRIV);
#endif
	pw = get_my_pwent();
	if (!pw) {
#ifdef USE_SYSLOG
		syslog(LOG_ERR, "can't get login name for uid %d.\n",
		       (int) getuid());
#endif
		fprintf(stderr, "Who are you?\n");
		exit(2);
	}
	strncpy(myname, pw->pw_name, sizeof myname - 1);
	myname[sizeof myname - 1] = '\0';
	name = myname;

	if (argc > 1) {
		name = argv[1];
		pw = getpwnam(name);
	}

	pass = get_line(stdin);
	if (password_auth_ok(pw, pass)) {
#ifdef USE_SYSLOG
		syslog(pw->pw_uid ? LOG_INFO : LOG_NOTICE,
		       "user `%s' entered correct password for `%.32s'.\n",
		       myname, name);
#endif
		exit(0);
	}
#ifdef USE_SYSLOG
	/* be careful not to overrun the syslog buffer */
	syslog((!pw || pw->pw_uid) ? LOG_NOTICE : LOG_WARNING,
	       "user `%s' entered incorrect password for `%.32s'.\n",
	       myname, name);
#endif
#ifdef FAIL_DELAY
	sleep(FAIL_DELAY);
#endif
	fprintf(stderr, "Wrong password.\n");
	exit(1);
}

#if 0
/*
 * You can use code similar to the following to run this program.
 * Return values: >=0 - program exit status (use the <sys/wait.h>
 * macros to get the exit code, it is shifted left by 8 bits),
 * -1 - check errno.
 */
int
verify_password(const char *username, const char *password)
{
	int pipe_fd[2];
	int pid, wpid, status;

	if (pipe(pipe_fd))
		return -1;
	
	if ((pid = fork()) == 0) {
		char *arg[3];
		char *env[1];

		/* child */
		close(pipe_fd[1]);
		if (pipe_fd[0] != 0) {
			if (dup2(pipe_fd[0], 0) != 0)
				_exit(127);
			close(pipe_fd[0]);
		}
		arg[0] = "/usr/bin/pwdauth";
		arg[1] = username;
		arg[2] = NULL;
		env[0] = NULL;
		execve(arg[0], arg, env);
		_exit(127);
	} else if (pid == -1) {
		/* error */
		close(pipe_fd[0]);
		close(pipe_fd[1]);
		return -1;
	}
	/* parent */
	close(pipe_fd[0]);
	write(pipe_fd[1], password, strlen(password));
	write(pipe_fd[1], "\n", 1);
	close(pipe_fd[1]);

	while ((wpid = wait(&status)) != pid) {
		if (wpid == -1)
			return -1;
	}
	return status;
}
#endif
