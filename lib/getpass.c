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
RCSID("$Id: getpass.c,v 1.11 2001/11/16 14:53:48 kloczek Exp $")

#include "defines.h"

#include <signal.h>
#include <stdio.h>

#include "getdef.h"

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


static char *
readpass(FILE *ifp, FILE *ofp, int with_echo, int max_asterisks)
{
	static char input[MAXLEN + 1], asterix[MAXLEN + 1];
	static char once;
	char *cp, *ap, c;
	int i;

	if (max_asterisks < 0) {
		/* traditional code using fgets() */
		if (fgets(input, sizeof input, ifp) != input)
			return NULL;
		cp = strrchr(input, '\n');
		if (cp)
			*cp = '\0';
		else
			input[sizeof input - 1] = '\0';
		return input;
	}
	if (!once) {
		srandom(time(0)*getpid());
		once = 1;
	}
	cp = input;
	ap = asterix;
	while (read(fileno(ifp), &c, 1)) {
		switch (c) {
		case '\n':
		case '\r':
			goto endwhile;
		case '\b':
		case 127:
			if (cp > input) {
				cp--;
				ap--;
				for (i = *ap; i > 0; i--)
					fputs("\b \b", ofp);
				*cp = '\0';
				*ap = 0;
			} else {
				putc('\a', ofp);  /* BEL */
			}
			break;
		case '\025':  /* Ctrl-U = erase everything typed so far */
			if (cp == input) {
				putc('\a', ofp);  /* BEL */
			} else while (cp > input) {
				cp--;
				ap--;
				for (i = *ap; i > 0; i--)
					fputs("\b \b", ofp);
				*cp = '\0';
				*ap = 0;
			}
			break;
		default:
			*cp++ = c;
			if (with_echo) {
				*ap = 1;
				putc(c, ofp);
			} else if (max_asterisks > 0) {
				*ap = (random() % max_asterisks) + 1;
				for (i = *ap; i > 0; i--)
					putc('*', ofp);
			} else {
				*ap = 0;
			}
			ap++;
			break;
		}
		fflush(ofp);
		if (cp >= input + MAXLEN) {
			putc('\a', ofp);  /* BEL */
			break;
		}
	}
endwhile:
	*cp = '\0';
	putc('\n', ofp);
	return input;
}

static char *
prompt_password(const char *prompt, int with_echo)
{
	static char nostring[1] = "";
	static char *return_value;
	volatile int tty_opened;
	static FILE *ifp, *ofp;
	volatile int is_tty;
#ifdef HAVE_SIGACTION
	struct sigaction old_sigact;
#else
	RETSIGTYPE (*old_signal)();
#endif
	TERMIO old_modes;
	int max_asterisks = getdef_num("GETPASS_ASTERISKS", -1);

	/*
	 * set a flag so the SIGINT signal can be re-sent if it
	 * is caught
	 */

	sig_caught = 0;
	return_value = NULL;
	tty_opened = 0;

	/*
	 * if /dev/tty can't be opened, getpass() needs to read
	 * from stdin and write to stderr instead.
	 */

	if (!(ifp = fopen("/dev/tty", "r+"))) {
		ifp = stdin;
		ofp = stderr;
	} else {
		ofp = ifp;
		tty_opened = 1;
	}
	setbuf(ifp, (char *) 0);

	/*
	 * the current tty modes must be saved so they can be
	 * restored later on.  echo will be turned off, except
	 * for the newline character
	 */

	is_tty = 1;
	if (GTTY(fileno(ifp), &old_modes)) {
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
	old_signal = signal(SIGINT, sig_catch);
#endif

	if (is_tty) {
		TERMIO new_modes = old_modes;

		if (max_asterisks < 0)
			new_modes.c_lflag |= ICANON;
		else
			new_modes.c_lflag &= ~(ICANON);

		if (with_echo)
			new_modes.c_lflag |= (ECHO | ECHOE | ECHOK);
		else
			new_modes.c_lflag &= ~(ECHO | ECHOE | ECHOK);

		new_modes.c_lflag |= ECHONL;

		if (STTY(fileno(ifp), &new_modes))
			goto out;
	}

	/*
	 * the prompt is output, and the response read without
	 * echoing.  the trailing newline must be removed.  if
	 * the fgets() returns an error, a NULL pointer is
	 * returned.
	 */

	if ((fputs(prompt, ofp) != EOF) && (fflush(ofp) != EOF))
		return_value = readpass(ifp, ofp, with_echo, max_asterisks);
out:
	/*
	 * the old SIGINT handler is restored after the tty
	 * modes.  then /dev/tty is closed if it was opened in
	 * the beginning.  finally, if a signal was caught it
	 * is sent to this process for normal processing.
	 */

	if (is_tty) {
		if (STTY(fileno(ifp), &old_modes))
			return_value = NULL;
	}

#ifdef HAVE_SIGACTION
	(void) sigaction (SIGINT, &old_sigact, NULL);
#else
	(void) signal (SIGINT, old_signal);
#endif
#if 0
out2:
#endif
	if (tty_opened)
		(void) fclose(ifp);

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

char *
libshadow_getpass(const char *prompt)
{
	return prompt_password(prompt, 0);
}

char *
getpass_with_echo(const char *prompt)
{
	return prompt_password(prompt, 1);
}

