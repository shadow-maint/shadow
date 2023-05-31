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

#include "alloc.h"
#include "run_part.h"
#include "shadowlog_internal.h"

#ifdef HAVE_EXECVEAT
int run_part (int script_fd, const char *script_name, const char *name, const char *action)
#else
int run_part (const char *script_path, const char *script_name, const char *name, const char *action)
#endif			/* HAVE_EXECVEAT */
{
	int pid;
	int wait_status;
	int pid_status;
	char *args[] = { (char *) script_name, NULL };

	pid=fork();
	if (pid==-1) {
		perror ("Could not fork");
		return 1;
	}
	if (pid==0) {
		setenv ("ACTION",action,1);
		setenv ("SUBJECT",name,1);
#ifdef HAVE_EXECVEAT
		execveat (script_fd, "", args, environ, AT_EMPTY_PATH);
#else
		execv (script_path, args);
#endif			/* HAVE_EXECVEAT */
		perror ("execv");
		exit(1);
	}

	pid_status = wait (&wait_status);
	if (pid_status == pid) {
		return (wait_status);
	}

	perror ("waitpid");
	return (1);
}

int run_parts (const char *directory, const char *name, const char *action)
{
	struct dirent **namelist;
	int scanlist;
	int n;
	int execute_result = 0;

#ifdef HAVE_EXECVEAT
	int dfd = open (directory, O_PATH | O_DIRECTORY | O_CLOEXEC);
	if (dfd == -1) {
		perror ("open");
		return (1);
	}
#endif			/* HAVE_EXECVEAT */

	scanlist = scandir (directory, &namelist, 0, alphasort);
	if (scanlist<=0) {
#ifdef HAVE_EXECVEAT
		(void) close (dfd);
#endif			/* HAVE_EXECVEAT */
		return (0);
	}

	for (n=0; n<scanlist; n++) {
		struct stat sb;

#ifdef HAVE_EXECVEAT
		int fd = openat (dfd, namelist[n]->d_name, O_PATH | O_CLOEXEC);
		if (fd == -1) {
			perror ("open");
			for (; n<scanlist; n++) {
				free (namelist[n]);
			}
			free (namelist);
			(void) close (dfd);
			return (1);
		}
#else
		int path_length=strlen(directory) + strlen(namelist[n]->d_name) + 2;
		char *s = MALLOCARRAY(path_length, char);
		if (!s) {
			fprintf (shadow_logfd, "could not allocate memory\n");
			for (; n<scanlist; n++) {
				free (namelist[n]);
			}
			free (namelist);
			return (1);
		}
		snprintf (s, path_length, "%s/%s", directory, namelist[n]->d_name);
#endif			/* HAVE_EXECVEAT */

#ifdef HAVE_EXECVEAT
		if (fstat (fd, &sb) == -1) {
#else
		if (stat (s, &sb) == -1) {
#endif			/* HAVE_EXECVEAT */
			perror ("stat");
#ifdef HAVE_EXECVEAT
			(void) close (fd);
			(void) close (dfd);
#else
			free (s);
#endif			/* HAVE_EXECVEAT */
			for (; n<scanlist; n++) {
				free (namelist[n]);
			}
			free (namelist);
			return (1);
		}

		if (!S_ISREG (sb.st_mode)) {
#ifdef HAVE_EXECVEAT
			(void) close (fd);
#else
			free (s);
#endif			/* HAVE_EXECVEAT */
			free (namelist[n]);
			continue;
		}

		if ((sb.st_uid != 0 && sb.st_uid != getuid()) ||
		    (sb.st_gid != 0 && sb.st_gid != getgid()) ||
		    (sb.st_mode & 0002)) {
			fprintf (shadow_logfd, "skipping %s due to insecure ownership/permission\n",
			         namelist[n]->d_name);
#ifdef HAVE_EXECVEAT
			(void) close (fd);
#else
			free (s);
#endif			/* HAVE_EXECVEAT */
			free (namelist[n]);
			continue;
		}

#ifdef HAVE_EXECVEAT
		execute_result = run_part (fd, namelist[n]->d_name, name, action);
		(void) close (fd);
#else
		execute_result = run_part (s, namelist[n]->d_name, name, action);
		free (s);
#endif			/* HAVE_EXECVEAT */

		if (execute_result!=0) {
			fprintf (shadow_logfd,
				"%s: did not exit cleanly.\n",
			    namelist[n]->d_name);
			for (; n<scanlist; n++) {
				free (namelist[n]);
			}
			break;
		}

		free (namelist[n]);
	}
	free (namelist);

#ifdef HAVE_EXECVEAT
	(void) close (dfd);
#endif			/* HAVE_EXECVEAT */

	return (execute_result);
}

