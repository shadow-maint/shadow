/*
 * Copyright 1990 - 1995, Julianne Frances Haugh
 * Copyright 1998, Pavel Machek <pavel@ucw.cz>
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
RCSID("$Id: getpass.c,v 1.9 1999/06/07 16:40:44 marekm Exp $")

#include "defines.h"

#include <signal.h>
#include <stdio.h>

/* new code, #undef if there are any problems...  */
#define USE_SETJMP 1

#ifdef USE_SETJMP
#include <setjmp.h>

static sigjmp_buf intr;  /* where to jump on SIGINT */
#endif

static	int	sig_caught;
#ifdef HAVE_SIGACTION
static	struct	sigaction sigact;
#endif

/*ARGSUSED*/
static RETSIGTYPE
sig_catch(int sig)
{
	sig_caught = 1;
#ifdef USE_SETJMP
	siglongjmp(intr, 1);
#endif
}

#define MAXLEN 127

#ifndef NEW_READPASS  /* ./configure --enable-readpass */
#define OLD_READPASS 1
#endif

#ifndef OLD_READPASS
static char *
readpass(FILE *fp)
{
	static char input[MAXLEN + 1], asterix[MAXLEN + 1];
	static char once;
	char *cp, *ap, c;
	int i;

	if (!once) {
		srandom(time(0)*getpid());
		once = 1;
	}
	cp = input;
	ap = asterix;
	while (read(fileno(fp), &c, 1)) {
		switch (c) {
		case '\n':
		case '\r':
			goto endwhile;
		case '\b':
		case 127:
			if (cp > input) {
				cp--; ap--;
				for (i = 0; i < (*ap); i++) {
					putc('\b', stdout);
					putc(' ', stdout);
					putc('\b', stdout);
				}
			} else
				putc('\a', stdout);  /* BEL */
			break;
		default:
			*cp++ = c;
			*ap++ = (random() % 4)+1;
			for (i = 0; i < (*(ap-1)); i++)
				putc('*', stdout);
			break;
		}
		fflush(stdout);
		if (cp == input + MAXLEN)
			break;
	}
endwhile:
	*cp = 0;
	putc('\n', stdout);
	return input;
}
#else
static char *
readpass(FILE *fp)
{
	static char input[MAXLEN + 1];
	char *cp;

	if (fgets(input, sizeof input, fp) == input) {
		if ((cp = strrchr(input, '\n')))
			*cp = '\0';
		else
			input[sizeof input - 1] = '\0';
#ifdef USE_SGTTY
		putc('\n', stdout);
#endif
		return input;
	}
	return NULL;
}
#endif

char *
libshadow_getpass(const char *prompt)
{
	static char nostring[1] = "";
	static char *return_value;
	volatile int tty_opened;
	static FILE *fp;
	volatile int is_tty;
#ifdef HAVE_SIGACTION
	struct sigaction old_sigact;
#else
	RETSIGTYPE (*old_signal)();
#endif
	TERMIO new_modes;
	TERMIO old_modes;

	/*
	 * set a flag so the SIGINT signal can be re-sent if it
	 * is caught
	 */

	sig_caught = 0;
	return_value = NULL;
	tty_opened = 0;

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

	is_tty = 1;
	if (GTTY(fileno(fp), &old_modes)) {
		is_tty = 0;
#if 0  /* to make getpass work with redirected stdin */
		return_value = NULL;
		goto out2;
#endif
	}

#ifdef USE_SETJMP
	/*
	 * If we get a SIGINT, sig_catch() will jump here -
	 * no need to press Enter after Ctrl-C.
	 */
	if (sigsetjmp(intr, 1))
		goto out;
#endif

#ifdef HAVE_SIGACTION
	sigact.sa_handler = sig_catch;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	sigaction(SIGINT, &sigact, &old_sigact);
#else
	old_signal = signal (SIGINT, sig_catch);
#endif

	new_modes = old_modes;

#ifdef USE_SGTTY
	new_modes.sg_flags &= ~ECHO ;
#else
#ifdef OLD_READPASS
	new_modes.c_lflag &= ~(ECHO|ECHOE|ECHOK);
#else
	new_modes.c_lflag &= ~(ECHO|ECHOE|ECHOK|ICANON);
#endif
	new_modes.c_lflag |= ECHONL;
#endif

	if (is_tty) {
		if (STTY(fileno(fp), &new_modes))
			goto out;
	}

	/*
	 * the prompt is output, and the response read without
	 * echoing.  the trailing newline must be removed.  if
	 * the fgets() returns an error, a NULL pointer is
	 * returned.
	 */

	if ((fputs(prompt, stdout) != EOF) && (fflush(stdout) != EOF))
		return_value = readpass(fp);
out:
	/*
	 * the old SIGINT handler is restored after the tty
	 * modes.  then /dev/tty is closed if it was opened in
	 * the beginning.  finally, if a signal was caught it
	 * is sent to this process for normal processing.
	 */

	if (is_tty) {
		if (STTY(fileno(fp), &old_modes))
			return_value = NULL;
	}

#ifdef HAVE_SIGACTION
	(void) sigaction (SIGINT, &old_sigact, NULL);
#else
	(void) signal (SIGINT, old_signal);
#endif
out2:
	if (tty_opened)
		(void) fclose (fp);

	if (sig_caught) {
		kill(getpid(), SIGINT);
		return_value = NULL;
	}
	if (!return_value) {
		nostring[0] = '\0';
		return_value = nostring;
	}
	return return_value;
}
