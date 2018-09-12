/* Author: Peter Vrabec <pvrabec@redhat.com> */

#include <config.h>
#ifdef USE_SSSD

#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "exitcodes.h"
#include "defines.h"
#include "prototypes.h"
#include "sssd.h"

#define MSG_SSSD_FLUSH_CACHE_FAILED "%s: Failed to flush the sssd cache.\n"

int sssd_flush_cache (int dbflags)
{
	int status, code, rv;
	const char *cmd = "/usr/sbin/sss_cache";
	char *sss_cache_args = NULL;
	const char *spawnedArgs[] = {"sss_cache", NULL, NULL};
	const char *spawnedEnv[] = {NULL};
	int i = 0;

	sss_cache_args = malloc(4);
	if (sss_cache_args == NULL) {
	    return -1;
	}

	sss_cache_args[i++] = '-';
	if (dbflags & SSSD_DB_PASSWD) {
		sss_cache_args[i++] = 'U';
	}
	if (dbflags & SSSD_DB_GROUP) {
		sss_cache_args[i++] = 'G';
	}
	sss_cache_args[i++] = '\0';
	if (i == 2) {
		/* Neither passwd nor group, nothing to do */
		free(sss_cache_args);
		return 0;
	}
	spawnedArgs[1] = sss_cache_args;

	rv = run_command (cmd, spawnedArgs, spawnedEnv, &status);
	free(sss_cache_args);
	if (rv != 0) {
		/* run_command writes its own more detailed message. */
		(void) fprintf (stderr, _(MSG_SSSD_FLUSH_CACHE_FAILED), Prog);
		return -1;
	}

	code = WEXITSTATUS (status);
	if (!WIFEXITED (status)) {
		(void) fprintf (stderr,
		                _("%s: sss_cache did not terminate normally (signal %d)\n"),
		                Prog, WTERMSIG (status));
		return -1;
	} else if (code == E_CMD_NOTFOUND) {
		/* sss_cache is not installed, or it is installed but uses an
		   interpreter that is missing.  Probably the former. */
		return 0;
	} else if (code != 0) {
		(void) fprintf (stderr, _("%s: sss_cache exited with status %d\n"),
		                Prog, code);
		(void) fprintf (stderr, _(MSG_SSSD_FLUSH_CACHE_FAILED), Prog);
		return -1;
	}

	return 0;
}
#else				/* USE_SSSD */
extern int errno;		/* warning: ANSI C forbids an empty source file */
#endif				/* USE_SSSD */

