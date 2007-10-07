/*
 * $Id: tar.c,v 1.2 1999/03/07 19:14:24 marekm Exp $
 *
 * This is a wrapper for tar to ensure that all files within the
 * newly created tar archive have the owner and group set to
 * root:root.  This makes it possible to build Debian packages
 * without root privileges (normally needed to chown files).
 * 
 * Assumptions:
 * - the directory containing this program is listed in $PATH
 * before the directory containing the real tar program (/bin)
 * - the options passed to tar cause it to output the archive
 * (not compressed) on standard output
 *
 * Written by Marek Michalkiewicz <marekm@linux.org.pl>,
 * public domain, no warranty, etc.
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <fnmatch.h>
#include <tar.h>

#ifndef REAL_TAR
#define REAL_TAR "/bin/tar"
#endif

#define RECORD_SIZE 512

union record {
	char data[RECORD_SIZE];
	struct header {
		char name[100];		/* NUL-terminated if NUL fits */
		char mode[8];	/* 0+ spaces, 1-6 octal digits, space, NUL */
		char uid[8];	/* format same as mode */
		char gid[8];	/* format same as mode */
		char size[12];	/* 0+ spaces, 1-11 octal digits, space */
				/* if '1' <= typeflag <= '6', ignore size */
		char mtime[12];	/* format same as size */
		char chksum[8];	/* 0+ spaces, 1-6 octal digits, NUL, space */
		char typeflag;
		char linkname[100];	/* NUL-terminated if NUL fits */
/* XXX - for GNU tar, magic is "ustar " (no NUL) and version is " \0" */
		char magic[6];		/* must be TMAGIC (NUL term.) */
		char version[2];	/* must be TVERSION */
		char uname[32];		/* NUL-terminated */
		char gname[32];		/* NUL-terminated */
		char devmajor[8];
		char devminor[8];
#ifdef GNU_TAR_FORMAT
		char atime[12];
		char ctime[12];
		char offset[12];
		char longnames[4];
		char pad;
		struct sparse {
			char offset[12];
			char numbytes[12];
		} sp[4];
		char isextended;
		char realsize[12];
#else
/* if prefix[0] != NUL then filename = prefix/name else filename = name */
		char prefix[155];	/* NUL-terminated if NUL fits */
#endif
	} h;
#ifdef GNU_TAR_FORMAT
	struct exthdr {
		struct sparse sp[21];
		char isextended;
	} xh;
#endif
};

static union record tarbuf;
static int infd = -1, outfd = -1;

int main(int, char **);
static ssize_t xread(int, char *, size_t);
static ssize_t xwrite(int, const char *, size_t);
static int block_is_eof(void);
static void block_read(void);
static void block_write(void);
static void verify_magic(void);
static void verify_checksum(void);
static void update_checksum(void);
static void set_owner(const char *);
static void set_group(const char *);
static void process_archive(void);


int
main(int argc, char **argv)
{
	int pipefd[2];
	pid_t pid;
	const char *real_tar;
	int status;

	real_tar = getenv("REAL_TAR");
	if (!real_tar)
		real_tar = REAL_TAR;
	if (pipe(pipefd)) {
		perror("pipe");
		exit(1);
	}
	pid = fork();
	if (pid == 0) {  /* child */
		/* redirect stdout to the pipe */
		if (dup2(pipefd[1], STDOUT_FILENO) != 1) {
			perror("dup2");
			_exit(126);
		}
		close(pipefd[0]);
		close(pipefd[1]);
		/* run the real tar program */
		execv(real_tar, argv);
		if (errno == ENOENT) {
			perror("execve");
			_exit(127);
		} else {
			perror("execve");
			_exit(126);
		}
	} else if (pid < 0) {  /* error */
		perror("fork");
		exit(1);
	}
	/* parent */
	close(pipefd[1]);
	/* read from pipefd[0], modify tar headers, write to stdout ... */
	infd = pipefd[0];
	outfd = STDOUT_FILENO;
	process_archive();
	/* wait for the tar subprocess to finish, and return its exit status */
	status = 1;
	if (waitpid(pid, &status, 0) == -1) {
		perror("waitpid");
		exit(1);
	}
	if (WIFSIGNALED(status)) {
		kill(getpid(), WTERMSIG(status));
		exit(1);
	}
	exit(WEXITSTATUS(status));
}

/* EINTR-safe versions of read() and write() - they don't really help much
   as GNU tar itself (version 1.11.8 at least) is not EINTR-safe, but it
   doesn't hurt...  Also, these functions never return errors - instead,
   they print an error message to stderr, and exit(1).  End of file is
   indicated by returning the number of bytes actually read.  */

static ssize_t
xread(int fd, char *buf, size_t count)
{
	ssize_t n;
	size_t left;

	left = count;
	do {
		n = read(fd, buf, left);
		if ((n < 0) && (errno == EINTR))
			continue;
		if (n <= 0)
			break;
		left -= n;
		buf += n;
	} while (left > 0);
	if (count > left)
		return count - left;
	if (n < 0) {
		perror("read");
		exit(1);
	}
	return 0;
}


static ssize_t
xwrite(int fd, const char *buf, size_t count)
{
	ssize_t n;
	size_t left;

	left = count;
	do {
		n = write(fd, buf, left);
		if (n < 0) {
			if (errno == EINTR)
				continue;
			/* any other write errors are fatal */
			perror("write");
			exit(1);
		}
		left -= n;
		buf += n;
	} while (left > 0);
	return count;
}


static int
block_is_eof(void)
{
	unsigned int i;

	for (i = 0; i < sizeof(tarbuf.data); i++) {
		if (tarbuf.data[i])
			return 0;
	}
	return 1;
}


static void
block_read(void)
{
	ssize_t nread;

	nread = xread(infd, tarbuf.data, RECORD_SIZE);
	if (nread != RECORD_SIZE) {
		fprintf(stderr, "unexpected end of file\n");
		exit(1);
	}
}


static void
block_write(void)
{
	xwrite(outfd, tarbuf.data, RECORD_SIZE);
}


static void
verify_magic(void)
{
	/* only check that magic starts with "ustar" - works for
	   standard UNIX tar as well as GNU tar formats.  */
	if (strncmp(tarbuf.h.magic, "ustar", 5) != 0) {
		fprintf(stderr, "bad tar header magic\n");
		exit(1);
	}
}


static void
verify_checksum(void)
{
	unsigned int i;
	int csum;

	if (sscanf(tarbuf.h.chksum, "%o", &csum) != 1) {
		fprintf(stderr, "bad tar checksum format\n");
		exit(1);
	}
	memset(tarbuf.h.chksum, ' ', sizeof(tarbuf.h.chksum));
	for (i = 0; i < sizeof(tarbuf.data); i++)
		csum -= (unsigned char) tarbuf.data[i];
	if (csum) {
		fprintf(stderr, "bad tar checksum value\n");
		exit(1);
	}
}


static void
update_checksum(void)
{
	unsigned int i;
	int csum;

	memset(tarbuf.h.chksum, ' ', sizeof(tarbuf.h.chksum));
	csum = 0;
	for (i = 0; i < sizeof(tarbuf.data); i++)
		csum += (unsigned char) tarbuf.data[i];
	snprintf(tarbuf.h.chksum, sizeof(tarbuf.h.chksum), "%6o", csum);
}


static void
set_owner(const char *username)
{
	const struct passwd *pw;

	pw = getpwnam(username);
	memset(tarbuf.h.uname, 0, sizeof(tarbuf.h.uname));
	snprintf(tarbuf.h.uname, sizeof(tarbuf.h.uname), "%s", username);
	snprintf(tarbuf.h.uid, sizeof(tarbuf.h.uid), "%6o ", (int) (pw ? pw->pw_uid : 0));
}


static void
set_group(const char *groupname)
{
	const struct group *gr;

	gr = getgrnam(groupname);
	memset(tarbuf.h.gname, 0, sizeof(tarbuf.h.gname));
	snprintf(tarbuf.h.gname, sizeof(tarbuf.h.gname), "%s", groupname);
	snprintf(tarbuf.h.gid, sizeof(tarbuf.h.gid), "%6o ", (int) (gr ? gr->gr_gid : 0));
}


static void
process_archive(void)
{
	ssize_t nread;
	long size;

	size = 0;
	for (;;) {
		/* read the header or data block */
		block_read();
		/* copy data blocks, if any */
		if (size > 0) {
			block_write();
			size -= RECORD_SIZE;
			continue;
		}
		if (block_is_eof()) {
			/* eof marker */
			block_write();
			break;
		}

		verify_magic();
		verify_checksum();

		/* process the header */
		switch (tarbuf.h.typeflag) {
		case LNKTYPE:
		case SYMTYPE:
		case CHRTYPE:
		case BLKTYPE:
		case DIRTYPE:
		case FIFOTYPE:
			/* no data blocks - ignore size */
			break;
		case REGTYPE:
		case AREGTYPE:
		case CONTTYPE:
		default:
			if (sscanf(tarbuf.h.size, "%lo", &size) != 1) {
				fprintf(stderr, "bad size format\n");
				exit(1);
			}
			break;
		}

		/* XXX - for now, just chown all files to root:root.  */
		set_owner("root");
		set_group("root");

		update_checksum();
		/* write the modified header */
		block_write();
	}
	/* eof marker detected, copy anything beyond it */
	for (;;) {
		nread = xread(infd, tarbuf.data, RECORD_SIZE);
		if (nread == 0)
			break;  /* end of file */
		xwrite(outfd, tarbuf.data, (size_t) nread);
	}
}

#if 0
/* permission specification file format, fixperms-1.00 compatible: 
 type filename owner group mode [linkname | major minor] [# comment]

 type:
  - = regular file
  l = link
  d = directory
  c = char dev
  b = block dev
  p = fifo
  s = socket

 filename - absolute pathname, wildcards ok [not for fixperms]
 linkname - only for type l
 major, minor - only for type c or b
 owner group - numeric, or names [not for fixperms]

 XXX not yet implemented
*/

struct permspec {
	char *name;
	uid_t uid;
	gid_t gid;
	mode_t mode;
	char *uname;
	char *gname;
	char *linkname;
	dev_t dev;
	struct permspec *next;
};
#endif
