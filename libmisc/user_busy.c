/*
 * Copyright (c) 1991 - 1994, Julianne Frances Haugh
 * Copyright (c) 1996 - 2000, Marek Michałkiewicz
 * Copyright (c) 2000 - 2006, Tomasz Kłoczko
 * Copyright (c) 2007 - 2009, Nicolas François
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
 * 3. The name of the copyright holders or contributors may not be used to
 *    endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <config.h>

#ident "$Id: $"

#include <assert.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include "defines.h"
#include "prototypes.h"
#ifdef ENABLE_SUBIDS
#include "subordinateio.h"
#endif				/* ENABLE_SUBIDS */
#include "shadowlog.h"

#ifdef __linux__
static int check_status (const char *name, const char *sname, uid_t uid);
static int user_busy_processes (const char *name, uid_t uid);
#else				/* !__linux__ */
static int user_busy_utmp (const char *name);
#endif				/* !__linux__ */

/*
 * user_busy - check if an user if currently running processes
 */
int user_busy (const char *name, uid_t uid)
{
	/* There are no standard ways to get the list of processes.
	 * An option could be to run an external tool (ps).
	 */
#ifdef __linux__
	/* On Linux, directly parse /proc */
	return user_busy_processes (name, uid);
#else				/* !__linux__ */
	/* If we cannot rely on /proc, check is there is a record in utmp
	 * indicating that the user is still logged in */
	return user_busy_utmp (name);
#endif				/* !__linux__ */
}

#ifndef __linux__
static int user_busy_utmp (const char *name)
{
#ifdef USE_UTMPX
	struct utmpx *utent;

	setutxent ();
	while ((utent = getutxent ()) != NULL)
#else				/* !USE_UTMPX */
	struct utmp *utent;

	setutent ();
	while ((utent = getutent ()) != NULL)
#endif				/* !USE_UTMPX */
	{
		if (utent->ut_type != USER_PROCESS) {
			continue;
		}
		if (strncmp (utent->ut_user, name, sizeof utent->ut_user) != 0) {
			continue;
		}
		if (kill (utent->ut_pid, 0) != 0) {
			continue;
		}

		fprintf (log_get_logfd(),
		         _("%s: user %s is currently logged in\n"),
		         log_get_progname(), name);
		return 1;
	}

	return 0;
}
#endif				/* !__linux__ */

#ifdef __linux__
#ifdef ENABLE_SUBIDS
#define in_parentuid_range(uid) ((uid) >= parentuid && (uid) < parentuid + range)
static int different_namespace (const char *sname)
{
	/* 41: /proc/xxxxxxxxxx/task/xxxxxxxxxx/ns/user + \0 */
	char path[41];
	char buf[512], buf2[512];
	ssize_t llen1, llen2;

	snprintf (path, 41, "/proc/%s/ns/user", sname);

	if ((llen1 = readlink (path, buf, sizeof(buf))) == -1)
		return 0;

	if ((llen2 = readlink ("/proc/self/ns/user", buf2, sizeof(buf2))) == -1)
		return 0;

	if (llen1 == llen2 && memcmp (buf, buf2, llen1) == 0)
		return 0; /* same namespace */

	return 1;
}
#endif                          /* ENABLE_SUBIDS */


static int check_status (const char *name, const char *sname, uid_t uid)
{
	/* 40: /proc/xxxxxxxxxx/task/xxxxxxxxxx/status + \0 */
	char status[40];
	char line[1024];
	FILE *sfile;

	snprintf (status, 40, "/proc/%s/status", sname);

	sfile = fopen (status, "r");
	if (NULL == sfile) {
		return 0;
	}
	while (fgets (line, sizeof (line), sfile) == line) {
		if (strncmp (line, "Uid:\t", 5) == 0) {
			unsigned long ruid, euid, suid;

			assert (uid == (unsigned long) uid);
			(void) fclose (sfile);
			if (sscanf (line,
			            "Uid:\t%lu\t%lu\t%lu\n",
			            &ruid, &euid, &suid) == 3) {
				if (   (ruid == (unsigned long) uid)
				    || (euid == (unsigned long) uid)
				    || (suid == (unsigned long) uid) ) {
					return 1;
				}
#ifdef ENABLE_SUBIDS
				if (    different_namespace (sname)
				     && (   have_sub_uids(name, ruid, 1)
				         || have_sub_uids(name, euid, 1)
				         || have_sub_uids(name, suid, 1))
				   ) {
					return 1;
				}
#endif				/* ENABLE_SUBIDS */
			} else {
				/* Ignore errors. This is just a best effort. */
			}
			return 0;
		}
	}
	(void) fclose (sfile);
	return 0;
}

static int user_busy_processes (const char *name, uid_t uid)
{
	DIR *proc;
	struct dirent *ent;
	char *tmp_d_name;
	pid_t pid;
	DIR *task_dir;
	/* 22: /proc/xxxxxxxxxx/task + \0 */
	char task_path[22];
	char root_path[22];
	struct stat sbroot;
	struct stat sbroot_process;

#ifdef ENABLE_SUBIDS
	sub_uid_open (O_RDONLY);
#endif				/* ENABLE_SUBIDS */

	proc = opendir ("/proc");
	if (proc == NULL) {
		perror ("opendir /proc");
#ifdef ENABLE_SUBIDS
		sub_uid_close();
#endif
		return 0;
	}
	if (stat ("/", &sbroot) != 0) {
		perror ("stat (\"/\")");
		(void) closedir (proc);
#ifdef ENABLE_SUBIDS
		sub_uid_close();
#endif
		return 0;
	}

	while ((ent = readdir (proc)) != NULL) {
		tmp_d_name = ent->d_name;
		/*
		 * Ingo Molnar's patch introducing NPTL for 2.4 hides
		 * threads in the /proc directory by prepending a period.
		 * This patch is applied by default in some RedHat
		 * kernels.
		 */
		if (   (strcmp (tmp_d_name, ".") == 0)
		    || (strcmp (tmp_d_name, "..") == 0)) {
			continue;
		}
		if (*tmp_d_name == '.') {
			tmp_d_name++;
		}

		/* Check if this is a valid PID */
		if (get_pid (tmp_d_name, &pid) == 0) {
			continue;
		}

		/* Check if the process is in our chroot */
		snprintf (root_path, 22, "/proc/%lu/root", (unsigned long) pid);
		root_path[21] = '\0';
		if (stat (root_path, &sbroot_process) != 0) {
			continue;
		}
		if (   (sbroot.st_dev != sbroot_process.st_dev)
		    || (sbroot.st_ino != sbroot_process.st_ino)) {
			continue;
		}

		if (check_status (name, tmp_d_name, uid) != 0) {
			(void) closedir (proc);
#ifdef ENABLE_SUBIDS
			sub_uid_close();
#endif
			fprintf (log_get_logfd(),
			         _("%s: user %s is currently used by process %d\n"),
			         log_get_progname(), name, pid);
			return 1;
		}

		snprintf (task_path, 22, "/proc/%lu/task", (unsigned long) pid);
		task_path[21] = '\0';
		task_dir = opendir (task_path);
		if (task_dir != NULL) {
			while ((ent = readdir (task_dir)) != NULL) {
				pid_t tid;
				if (get_pid (ent->d_name, &tid) == 0) {
					continue;
				}
				if (tid == pid) {
					continue;
				}
				if (check_status (name, task_path+6, uid) != 0) {
					(void) closedir (proc);
					(void) closedir (task_dir);
#ifdef ENABLE_SUBIDS
					sub_uid_close();
#endif
					fprintf (log_get_logfd(),
					         _("%s: user %s is currently used by process %d\n"),
					         log_get_progname(), name, pid);
					return 1;
				}
			}
			(void) closedir (task_dir);
		} else {
			/* Ignore errors. This is just a best effort */
		}
	}

	(void) closedir (proc);
#ifdef ENABLE_SUBIDS
	sub_uid_close();
#endif				/* ENABLE_SUBIDS */
	return 0;
}
#endif				/* __linux__ */

