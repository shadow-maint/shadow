/*
 * Copyright 1990, 1991, 1993, John F. Haugh II
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

#include <signal.h>
#include <stdio.h>
#include "config.h"

#ifdef	BSD
#include <sgtty.h>
#include <strings.h>
#else
#ifdef	SVR4
#include <termios.h>
#else
#include <termio.h>
#endif	/* SVR4 */
#include <string.h>
#endif

#ifndef	lint
static	char	sccsid[] = "@(#)getpass.c	3.8	07:26:36	23 Apr 1993";
#endif

/*
 * limits.h may be kind enough to specify the length of a prompted
 * for password.
 */

#if __STDC__ || _POSIX_SOURCE
#include <limits.h>
#endif

/*
 * This is really a giant mess.  On the one hand, it would be nice
 * if PASS_MAX were real big so that DOUBLESIZE isn't needed.  But
 * if it is defined we must honor it because some idiot might use
 * this in a routine expecting some standard behavior.
 */

#ifndef	PASS_MAX
#ifdef	SW_CRYPT
#define	PASS_MAX	80
#else	/* !SW_CRYPT */
#ifdef	DOUBLESIZE
#define	PASS_MAX	16
#else	/* !PASS_MAX */
#define	PASS_MAX	8
#endif	/* PASS_MAX */
#endif	/* SW_CRYPT */
#endif	/* !PASS_MAX */

#ifdef	BSD
#define	STTY(fd,termio)	stty(fd, termio)
#define	GTTY(fd,termio) gtty(fd, termio)
#define	TERMIO	struct	sgttyb
#define	INDEX	index
#else
#ifdef	SVR4
#define	STTY(fd,termio) ioctl(fd, TCSETS, termio)
#define	GTTY(fd,termio) ioctl(fd, TCGETS, termio)
#define	TERMIO	struct	termios
#else
#define	STTY(fd,termio) ioctl(fd, TCSETA, termio)
#define	GTTY(fd,termio) ioctl(fd, TCGETA, termio)
#define	TERMIO	struct	termio
#endif
#define	INDEX	strchr
#endif

static	int	sig_caught;

static void
sig_catch ()
{
	sig_caught = 1;
}

char *
getpass (prompt)
char	*prompt;
{
	static	char	input[PASS_MAX+1];
	char	*return_value = 0;
	char	*cp;
	FILE	*fp;
	int	tty_opened = 0;
	SIGTYPE	(*old_signal)();
	TERMIO	new_modes;
	TERMIO	old_modes;

	/*
	 * set a flag so the SIGINT signal can be re-sent if it
	 * is caught
	 */

	sig_caught = 0;

	/*
	 * if /dev/tty can't be opened, getpass() needs to read
	 * from stdin instead.
	 */

	if ((fp = fopen ("/dev/tty", "r")) == 0) {
		fp = stdin;
		setbuf (fp, (char *) 0);
	} else {
		tty_opened = 1;
	}

	/*
	 * the current tty modes must be saved so they can be
	 * restored later on.  echo will be turned off, except
	 * for the newline character (BSD has to punt on this)
	 */

	if (GTTY (fileno (fp), &new_modes))
		return 0;

	old_modes = new_modes;
	old_signal = signal (SIGINT, sig_catch);

#ifdef	BSD
	new_modes.sg_flags &= ~ECHO ;
#else
	new_modes.c_lflag &= ~(ECHO|ECHOE|ECHOK);
	new_modes.c_lflag |= ECHONL;
#endif

	if (STTY (fileno (fp), &new_modes))
		goto out;

	/*
	 * the prompt is output, and the response read without
	 * echoing.  the trailing newline must be removed.  if
	 * the fgets() returns an error, a NULL pointer is
	 * returned.
	 */

	if (fputs (prompt, stdout) == EOF)
		goto out;

	(void) fflush (stdout);

	if (fgets (input, sizeof input, fp) == input) {
		if (cp = INDEX (input, '\n'))
			*cp = '\0';
		else
			input[sizeof input - 1] = '\0';

		return_value = input;
#ifdef	BSD
		putc ('\n', stdout);
#endif
	}
out:
	/*
	 * the old SIGINT handler is restored after the tty
	 * modes.  then /dev/tty is closed if it was opened in
	 * the beginning.  finally, if a signal was caught it
	 * is sent to this process for normal processing.
	 */

	if (STTY (fileno (fp), &old_modes))
		return_value = 0;

	(void) signal (SIGINT, old_signal);

	if (tty_opened)
		(void) fclose (fp);

	if (sig_caught) {
		kill (getpid (), SIGINT);
		return_value = 0;
	}
	return return_value;
}
