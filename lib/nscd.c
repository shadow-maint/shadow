/* Author: Peter Vrabec <pvrabec@redhat.com> */

#include <config.h>
#ifdef USE_NSCD

#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "exitcodes.h"
#include "defines.h"
#include "prototypes.h"
#include "nscd.h"
#include "shadowlog_internal.h"

#define MSG_NSCD_FLUSH_CACHE_FAILED "%s: Failed to flush the nscd cache.\n"

/*
 * nscd_flush_cache - flush specified service buffer in nscd cache
 */
int nscd_flush_cache (const char *service)
{
	int status, code;
	const char *cmd = "/usr/sbin/nscd";
	const char *spawnedArgs[] = {"nscd", "-i", service, NULL};
	const char *spawnedEnv[] = {NULL};

	if (run_command (cmd, spawnedArgs, spawnedEnv, &status) != 0) {
		/* run_command writes its own more detailed message. */
		(void) fprintf (shadow_logfd, _(MSG_NSCD_FLUSH_CACHE_FAILED), Prog);
		return -1;
	}

	code = WEXITSTATUS (status);
	if (!WIFEXITED (status)) {
		(void) fprintf (shadow_logfd,
		                _("%s: nscd did not terminate normally (signal %d)\n"),
		                Prog, WTERMSIG (status));
		return -1;
	} else if (code == E_CMD_NOTFOUND) {
		/* nscd is not installed, or it is installed but uses an
		   interpreter that is missing.  Probably the former. */
		return 0;
	} else if (code == 1) {
		/* nscd is installed, but it isn't active. */
		return 0;
	} else if (code != 0) {
		(void) fprintf (shadow_logfd, _("%s: nscd exited with status %d\n"),
		                Prog, code);
		(void) fprintf (shadow_logfd, _(MSG_NSCD_FLUSH_CACHE_FAILED), Prog);
		return -1;
	}

	return 0;
}
#else				/* USE_NSCD */
extern int errno;		/* warning: ANSI C forbids an empty source file */
#endif				/* USE_NSCD */

