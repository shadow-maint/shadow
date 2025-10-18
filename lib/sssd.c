/* Author: Peter Vrabec <pvrabec@redhat.com> */

#include "config.h"

#ifdef USE_SSSD
#include "sssd.h"

#include <stdio.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "alloc/malloc.h"
#include "exitcodes.h"
#include "defines.h"
#include "prototypes.h"
#include "shadowlog_internal.h"
#include "string/strcmp/streq.h"


#define MSG_SSSD_FLUSH_CACHE_FAILED "%s: Failed to flush the sssd cache."


int
sssd_flush_cache(int dbflags)
{
	int          status, code, rv;
	char         *p;
	char         *sss_cache_args = NULL;
	const char   *cmd = "/usr/sbin/sss_cache";
	const char   *spawnedArgs[] = {"sss_cache", NULL, NULL};
	const char   *spawnedEnv[] = {NULL};
	struct stat  sb;

	if ((dbflags & (SSSD_DB_PASSWD|SSSD_DB_GROUP)) == 0)
	    /* Neither passwd nor group, nothing to do */
	    return 0;

	rv = stat(cmd, &sb);
	if (rv == -1 && errno == ENOENT)
		return 0;

	sss_cache_args = MALLOC(4, char);
	if (sss_cache_args == NULL) {
	    return -1;
	}

	p = stpcpy(sss_cache_args, "-");
	if (dbflags & SSSD_DB_PASSWD)
		p = stpcpy(p, "U");
	if (dbflags & SSSD_DB_GROUP)
		p = stpcpy(p, "G");

	spawnedArgs[1] = sss_cache_args;

	rv = run_command (cmd, spawnedArgs, spawnedEnv, &status);
	free(sss_cache_args);
	if (rv != 0) {
		/* run_command writes its own more detailed message. */
		SYSLOG ((LOG_WARN, MSG_SSSD_FLUSH_CACHE_FAILED, shadow_progname));
		return -1;
	}

	code = WEXITSTATUS (status);
	if (!WIFEXITED (status)) {
		SYSLOG ((LOG_WARN, "%s: sss_cache did not terminate normally (signal %d)",
			shadow_progname, WTERMSIG (status)));
		return -1;
	} else if (code == E_CMD_NOTFOUND) {
		/* sss_cache is not installed, or it is installed but uses an
		   interpreter that is missing.  Probably the former. */
		return 0;
	} else if (code != 0) {
		SYSLOG ((LOG_WARN, "%s: sss_cache exited with status %d", shadow_progname, code));
		SYSLOG ((LOG_WARN, MSG_SSSD_FLUSH_CACHE_FAILED, shadow_progname));
		return -1;
	}

	return 0;
}
#endif				/* USE_SSSD */
