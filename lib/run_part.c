#include <config.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <lib/prototypes.h>

#include "run_part.h"
#include "shadowlog_internal.h"


static int run_part(int script_fd, const char *script_name, const char *name, const char *action)
{
	pid_t pid;
	int wait_status;
	pid_t pid_status;
	char *args[] = { (char *) script_name, NULL };

	pid=fork();
	if (pid==-1) {
		fprintf(shadow_logfd, "fork: %s\n", strerror(errno));
		return 1;
	}
	if (pid==0) {
		setenv("ACTION",action,1);
		setenv("SUBJECT",name,1);
		fexecve(script_fd, args, environ);
		fprintf(shadow_logfd, "fexecve(%s): %s\n", script_name, strerror(errno));
		exit(1);
	}

	pid_status = wait(&wait_status);
	if (pid_status == pid) {
		return (wait_status);
	}

	fprintf(shadow_logfd, "failed to wait for pid %d (%s): %s\n", pid, script_name, strerror(errno));
	return (1);
}

int run_parts(const char *directory, const char *name, const char *action)
{
	struct dirent **namelist;
	int scanlist;
	int n;
	int execute_result = 0;

	int dfd = open(directory, O_PATH | O_DIRECTORY | O_CLOEXEC);
	if (dfd == -1) {
		fprintf(shadow_logfd, "failed open directory %s: %s\n", directory, strerror(errno));
		return (1);
	}

#ifdef HAVE_SCANDIRAT
	scanlist = scandirat(dfd, ".", &namelist, 0, alphasort);
#else
	scanlist = scandir(directory, &namelist, 0, alphasort);
#endif
	if (scanlist<=0) {
		(void) close(dfd);
		return (0);
	}

	for (n=0; n<scanlist; n++) {
		struct stat sb;

		int fd = openat (dfd, namelist[n]->d_name, O_PATH | O_CLOEXEC);
		if (fd == -1) {
			fprintf(shadow_logfd, "failed to open %s/%s: %s\n",
				directory, namelist[n]->d_name, strerror(errno));
			for (int k = n; k < scanlist; k++) {
				free(namelist[k]);
			}
			free(namelist);
			(void) close(dfd);
			return (1);
		}

		if (fstat (fd, &sb) == -1) {
			fprintf(shadow_logfd, "failed to stat %s/%s: %s\n",
				directory, namelist[n]->d_name, strerror(errno));
			(void) close(fd);
			(void) close(dfd);
			for (int k = n; k < scanlist; k++) {
				free(namelist[k]);
			}
			free(namelist);
			return (1);
		}

		if (!S_ISREG(sb.st_mode)) {
			(void) close(fd);
			free(namelist[n]);
			continue;
		}

		if ((sb.st_uid != 0 && sb.st_uid != getuid()) ||
		    (sb.st_gid != 0 && sb.st_gid != getgid()) ||
		    (sb.st_mode & 0002)) {
			fprintf(shadow_logfd, "skipping %s/%s due to insecure ownership/permission\n",
			         directory, namelist[n]->d_name);
			(void) close(fd);
			free(namelist[n]);
			continue;
		}

		execute_result = run_part(fd, namelist[n]->d_name, name, action);
		(void) close(fd);
		if (execute_result!=0) {
			fprintf(shadow_logfd,
				"%s/%s: did not exit cleanly.\n",
				directory, namelist[n]->d_name);
			for (int k = n; k < scanlist; k++) {
				free(namelist[k]);
			}
			break;
		}

		free(namelist[n]);
	}
	free(namelist);

	(void) close(dfd);

	return (execute_result);
}
