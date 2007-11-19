/* Author: Peter Vrabec <pvrabec@redhat.com> */

/* because of TEMP_FAILURE_RETRY */
#define _GNU_SOURCE

#include <features.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <spawn.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>


/*
 * nscd_flush_cache - flush specified service buffer in nscd cache
 */
int nscd_flush_cache (char *service)
{
	pid_t pid, termpid;
	int err, status;
	char *spawnedArgs[] = {"/usr/sbin/nscd", "nscd", "-i", service, NULL};
	char *spawnedEnv[] = {NULL};

	/* spawn process */
	if( (err=posix_spawn(&pid, spawnedArgs[0], NULL, NULL,
			     spawnedArgs, spawnedEnv)) !=0 ) 
	{
		fprintf(stderr, "posix_spawn() error=%d\n", err);
		return -1;
	}

	/* Wait for the spawned process to exit */	
	termpid = TEMP_FAILURE_RETRY (waitpid (pid, &status, 0));
	if (termpid == -1)
	{
		perror("waitpid");
		return -1;
	}
	else if (termpid != pid)
	{
		fprintf(stderr, "waitpid returned %ld != %ld\n",
			 (long int) termpid, (long int) pid);
		return -1;
	}

	return 0;
}

