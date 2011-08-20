/* Author: Peter Vrabec <pvrabec@redhat.com> */

#include <config.h>
#ifdef USE_NSCD

/* because of TEMP_FAILURE_RETRY */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <features.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <spawn.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "exitcodes.h"
#include "defines.h"
#include "spawn.h"
#include "nscd.h"

#define MSG_NSCD_FLUSH_CACHE_FAILED "Failed to flush the nscd cache.\n"

/*
 * nscd_flush_cache - flush specified service buffer in nscd cache
 */
int nscd_flush_cache (const char *service)
{
	pid_t pid;
	int err, status, code;
	const char *spawnedArgs[] = {"/usr/sbin/nscd", "nscd", "-i", service, NULL};
	const char *spawnedEnv[] = {NULL};

	err = run_command (spawnedArgs[0], spawnedArgs, spawnedEnv, &status);
	if (0 != err)
	{
		/* run_command writes its own more detailed message. */
		(void) fputs (_(MSG_NSCD_FLUSH_CACHE_FAILED), stderr);
		return -1;
	}
	code = WIFEXITED (status) ? WEXITSTATUS (status)
	                          : (WTERMSIG (status) + 128);
	if (code == E_CMD_NOTFOUND)
	{
		/* nscd is not installed, or it is installed but uses an
		   interpreter that is missing.  Probably the former. */
		return 0;
	}
	if (code != 0)
	{
		(void) fprintf (stderr, "nscd exited with status %d", code);
		(void) fputs (_(MSG_NSCD_FLUSH_CACHE_FAILED), stderr);
		return -1;
	}
	return 0;
}
#else				/* USE_NSCD */
extern int errno;		/* warning: ANSI C forbids an empty source file */
#endif				/* USE_NSCD */

