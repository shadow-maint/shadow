/*
 * Copyright (c) 2001 Rafal Wojtczuk, Solar Designer
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted.
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

#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <tcb.h>
#include <unistd.h>

#include "config.h"

#include "defines.h"
#include "getdef.h"

#define SHADOWTCB_HASH_BY 1000
#define SHADOWTCB_LOCK_SUFFIX ".lock"

static char *stored_tcb_user = NULL;

int shadowtcb_drop_priv()
{
	if (!getdef_bool("USE_TCB"))
		return 1;
	
	if (stored_tcb_user)
		return !tcb_drop_priv(stored_tcb_user);
	
	return 0;
}

int shadowtcb_gain_priv()
{
	if (!getdef_bool("USE_TCB"))
		return 1;
	return !tcb_gain_priv();
}

/* In case something goes wrong, we return immediately, not polluting the
 * code with free(). All errors are fatal, so the application is expected
 * to exit soon.
 */
#define OUT_OF_MEMORY do { \
	fprintf(stderr, _("%s: out of memory\n"), Prog); \
	fflush(stderr); \
	return 0; \
} while(0)

/* Returns user's tcb directory path relative to TCB_DIR. */
static char *shadowtcb_path_rel(const char *name, uid_t uid)
{
	char *ret;

	if (!getdef_bool("TCB_SYMLINKS") || uid < SHADOWTCB_HASH_BY) {
		asprintf(&ret, "%s", name);
	} else if (uid < SHADOWTCB_HASH_BY * SHADOWTCB_HASH_BY) {
		asprintf(&ret, ":%dK/%s", uid / SHADOWTCB_HASH_BY, name);
	} else {
		asprintf(&ret, ":%dM/:%dK/%s",
			uid / (SHADOWTCB_HASH_BY * SHADOWTCB_HASH_BY),
			(uid % (SHADOWTCB_HASH_BY * SHADOWTCB_HASH_BY)) / SHADOWTCB_HASH_BY,
			name);
	}
	if (!ret) {
		OUT_OF_MEMORY;
	}
	return ret;
}

static char *shadowtcb_path_rel_existing(const char *name)
{
	char *path, *rval;
	struct stat st;
	char link[8192];
	int ret;

	asprintf(&path, TCB_DIR "/%s", name);
	if (!path) {
		OUT_OF_MEMORY;
	}
	if (lstat(path, &st)) {
		fprintf(stderr, _("%s: Cannot stat %s: %s\n"), Prog, path, strerror(errno));
		free(path);
		return NULL;
	}
	if (S_ISDIR(st.st_mode)) {
		free(path);
		rval = strdup(name);
		if (!rval) {
			OUT_OF_MEMORY;
		}
		return rval;
	}
	if (!S_ISLNK(st.st_mode)) {
		fprintf(stderr, _("%s: %s is neither a directory, nor a symlink.\n"), Prog, path);
		free(path);
		return NULL;
	}
	ret = readlink(path, link, sizeof(link) - 1);
	free(path);
	if (ret == -1) {
		fprintf(stderr, _("%s: Cannot read symbolic link %s: %s\n"), Prog, path, strerror(errno));
		return NULL;
	}
	if (ret >= sizeof(link) - 1) {
		link[sizeof(link) - 1] = '\0';
		fprintf(stderr, _("%s: Suspiciously long symlink: %s\n"), Prog, link);
		return NULL;
	}
	link[ret] = '\0';
	rval = strdup(link);
	if (!rval) {
		OUT_OF_MEMORY;
	}
	return rval;
}

static char *shadowtcb_path(const char *name, uid_t uid)
{
	char *ret, *rel;

	if (!(rel = shadowtcb_path_rel(name, uid)))
		return 0;
	asprintf(&ret, TCB_DIR "/%s", rel);
	free(rel);
	if (!ret) {
		OUT_OF_MEMORY;
	}
	return ret;
}

static char *shadowtcb_path_existing(const char *name)
{
	char *ret, *rel;

	if (!(rel = shadowtcb_path_rel_existing(name)))
		return 0;
	asprintf(&ret, TCB_DIR "/%s", rel);
	free(rel);
	if (!ret) {
		OUT_OF_MEMORY;
	}
	return ret;
}

static int mkdir_leading(const char *name, uid_t uid)
{
	char *ind, *dir, *ptr, *path = shadowtcb_path_rel(name, uid);
	struct stat st;

	if (!path)
		return 0;
	ptr = path;
	if (stat(TCB_DIR, &st)) {
		fprintf(stderr, _("%s: Cannot stat %s: %s\n"), Prog, TCB_DIR, strerror(errno));
		goto out_free_path;
	}
	while ((ind = strchr(ptr, '/'))) {
		*ind = 0;
		asprintf(&dir, TCB_DIR "/%s", path);
		if (!dir) {
			OUT_OF_MEMORY;
		}
		if (mkdir(dir, 0700) && errno != EEXIST) {
			fprintf(stderr, _("%s: Cannot create directory %s: %s\n"), Prog, dir, strerror(errno));
			goto out_free_dir;
		}
		if (chown(dir, 0, st.st_gid)) {
			fprintf(stderr, _("%s: Cannot change owner of %s: %s\n"), Prog, dir, strerror(errno));
			goto out_free_dir;
		}
		if (chmod(dir, 0711)) {
			fprintf(stderr, _("%s: Cannot change mode of %s: %s\n"), Prog, dir, strerror(errno));
			goto out_free_dir;
		}
		free(dir);
		*ind = '/';
		ptr = ind + 1;
	}
	free(path);
	return 1;
out_free_dir:
	free(dir);
out_free_path:
	free(path);
	return 0;
}

static int unlink_suffs(const char *user)
{
	static char *suffs[] = { "+", "-", SHADOWTCB_LOCK_SUFFIX };
	char *tmp;
	int i;

	for (i = 0; i < 3; i++) {
		asprintf(&tmp, TCB_FMT "%s", user, suffs[i]);
		if (!tmp) {
			OUT_OF_MEMORY;
		}
		if (unlink(tmp) && errno != ENOENT) {
			fprintf(stderr, _("%s: unlink: %s: %s\n"), Prog, tmp, strerror(errno));
			free(tmp);
			return 0;
		}
		free(tmp);
	}

	return 1;
}

/* path should be a relative existing tcb directory */
static int rmdir_leading(char *path)
{
	char *ind, *dir;
	int ret = 1;
	while ((ind = strrchr(path, '/'))) {
		*ind = 0;
		asprintf(&dir, TCB_DIR "/%s", path);
		if (!dir) {
			OUT_OF_MEMORY;
		}
		if (rmdir(dir)) {
			if (errno != ENOTEMPTY) {
				fprintf(stderr, _("%s: Cannot removedirectory %s: %s\n"), Prog, dir, strerror(errno));
				ret = 0;
			}
			free(dir);
			break;
		}
		free(dir);
	}
	return ret;
}

static int move_dir(const char *user_newname, uid_t user_newid)
{
	char *olddir = NULL, *newdir = NULL;
	char *real_old_dir = NULL, *real_new_dir = NULL;
	char *real_old_dir_rel = NULL, *real_new_dir_rel = NULL;
	uid_t old_uid, the_newid;
	struct stat oldmode;
	int ret = 0;

	asprintf(&olddir, TCB_DIR "/%s", stored_tcb_user);
	if (!olddir)
		goto out_free_nomem;
	if (stat(olddir, &oldmode)) {
		fprintf(stderr, _("%s: Cannot stat %s: %s\n"), Prog, olddir, strerror(errno));
		goto out_free;
	}
	old_uid = oldmode.st_uid;
	the_newid = (user_newid == -1) ? old_uid : user_newid;
	if (!(real_old_dir = shadowtcb_path_existing(stored_tcb_user)))
		goto out_free;
	if (!(real_new_dir = shadowtcb_path(user_newname, the_newid)))
		goto out_free;
	if (!strcmp(real_old_dir, real_new_dir)) {
		ret = 1;
		goto out_free;
	}
	if (!(real_old_dir_rel = shadowtcb_path_rel_existing(stored_tcb_user)))
		goto out_free;
	if (!mkdir_leading(user_newname, the_newid))
		goto out_free;
	if (rename(real_old_dir, real_new_dir)) {
		fprintf(stderr, _("%s: Cannot rename %s to %s: %s\n"), Prog, real_old_dir, real_new_dir, strerror(errno));
		goto out_free;
	}
	if (!rmdir_leading(real_old_dir_rel))
		goto out_free;
	if (unlink(olddir) && errno != ENOENT) {
		fprintf(stderr, _("%s: Cannot remove %s: %s\n"), Prog, olddir, strerror(errno));
		goto out_free;
	}
	asprintf(&newdir, TCB_DIR "/%s", user_newname);
	if (!newdir)
		goto out_free_nomem;
	if (!(real_new_dir_rel = shadowtcb_path_rel(user_newname, the_newid)))
		goto out_free;
	if (strcmp(real_new_dir, newdir) && symlink(real_new_dir_rel, newdir)) {
		fprintf(stderr, _("%s: Cannot create symbolic link %s: %s\n"), Prog, real_new_dir_rel, strerror(errno));
		goto out_free;
	}
	ret = 1;
	goto out_free;
out_free_nomem:
	fprintf(stderr, _("%s: out of memory\n"), Prog); \
	fflush(stderr);
out_free:
	free(olddir);
	free(newdir);
	free(real_old_dir);
	free(real_new_dir);
	free(real_old_dir_rel);
	free(real_new_dir_rel);
	return ret;
}

int shadowtcb_set_user(const char* name)
{
	char *buf;
	int retval;

	if (!getdef_bool("USE_TCB"))
		return 1;
	
	if (stored_tcb_user)
		free(stored_tcb_user);
	
	stored_tcb_user = strdup(name);
	if (!stored_tcb_user) {
		OUT_OF_MEMORY;
	}
	asprintf(&buf, TCB_FMT, name);
	if (!buf) {
		OUT_OF_MEMORY;
	}

	retval = spw_setdbname(buf);
	free(buf);
	return retval;
}

/* tcb directory must be empty before shadowtcb_remove is called. */
int shadowtcb_remove(const char *name)
{
	int ret = 1;
	char *path = shadowtcb_path_existing(name);
	char *rel = shadowtcb_path_rel_existing(name);
	if (!path || !rel || rmdir(path))
		return 0;
	if (!rmdir_leading(rel))
		return 0;
	free(path);
	free(rel);
	asprintf(&path, TCB_DIR "/%s", name);
	if (!path) {
		OUT_OF_MEMORY;
	}
	if (unlink(path) && errno != ENOENT)
		ret = 0;
	free(path);
	return ret;
}

int shadowtcb_move(const char *user_newname, uid_t user_newid)
{
	struct stat dirmode, filemode;
	char *tcbdir, *shadow;
	int ret = 0;

	if (!getdef_bool("USE_TCB"))
		return 1;
	if (!user_newname)
		user_newname = stored_tcb_user;
	if (!move_dir(user_newname, user_newid))
		return 0;
	if (user_newid == -1)
		return 1;
	asprintf(&tcbdir, TCB_DIR "/%s", user_newname);
	asprintf(&shadow, TCB_FMT, user_newname);
	if (!tcbdir || !shadow) {
		OUT_OF_MEMORY;
	}
	if (stat(tcbdir, &dirmode)) {
		fprintf(stderr, _("%s: Cannot stat %s: %s\n"), Prog, tcbdir, strerror(errno));
		goto out_free;
	}
	if (chown(tcbdir, 0, 0)) {
		fprintf(stderr, _("%s: Cannot change owners of %s: %s\n"), Prog, tcbdir, strerror(errno));
		goto out_free;
	}
	if (chmod(tcbdir, 0700)) {
		fprintf(stderr, _("%s: Cannot change mode of %s: %s\n"), Prog, tcbdir, strerror(errno));
		goto out_free;
	}
	if (lstat(shadow, &filemode)) {
		if (errno != ENOENT) {
			fprintf(stderr, _("%s: Cannot lstat %s: %s\n"), Prog, shadow, strerror(errno));
			goto out_free;
		}
		fprintf(stderr,
			_("%s: Warning, user %s has no tcb shadow file.\n"),
			Prog, user_newname);
	} else {
		if (!S_ISREG(filemode.st_mode) ||
			filemode.st_nlink != 1) {
			fprintf(stderr,
				_("%s: Emergency: %s's tcb shadow is not a regular file"
				" with st_nlink=1.\n"
				"The account is left locked.\n"),
				Prog, user_newname);
			goto out_free;
		}
		if (chown(shadow, user_newid, filemode.st_gid)) {
			fprintf(stderr, _("%s: Cannot change owner of %s: %s\n"), Prog, shadow, strerror(errno));
			goto out_free;
		}
		if (chmod(shadow, filemode.st_mode & 07777)) {
			fprintf(stderr, _("%s: Cannot change mode of %s: %s\n"), Prog, shadow, strerror(errno));
			goto out_free;
		}
	}
	if (!unlink_suffs(user_newname))
		goto out_free;
	if (chown(tcbdir, user_newid, dirmode.st_gid)) {
		fprintf(stderr, _("%s: Cannot change owner of %s: %s\n"), Prog, tcbdir, strerror(errno));
		goto out_free;
	}
	ret = 1;
out_free:
	free(tcbdir);
	free(shadow);
	return ret;
}

int shadowtcb_create(const char *name, uid_t uid)
{
	char *dir, *shadow;
	struct stat tcbdir_stat;
	gid_t shadowgid, authgid;
	struct group *gr;
	int fd, ret = 0;

	if (!getdef_bool("USE_TCB"))
		return 1;
	if (stat(TCB_DIR, &tcbdir_stat)) {
		fprintf(stderr, _("%s: Cannot stat %s: %s\n"), Prog, tcbdir, strerror(errno));
		return 0;
	}
	shadowgid = tcbdir_stat.st_gid;
	if (getdef_bool("TCB_AUTH_GROUP") &&
		(gr = getgrnam("auth"))) {
		authgid = gr->gr_gid;
	} else {
		authgid = shadowgid;
	}
	
	asprintf(&dir, TCB_DIR "/%s", name);
	asprintf(&shadow, TCB_FMT, name);
	if (!dir || !shadow) {
		OUT_OF_MEMORY;
	}
	if (mkdir(dir, 0700)) {
		fprintf(stderr, _("%s: mkdir: %s: %s\n"), Prog, dir, strerror(errno));
		goto out_free;
		return 0;
	}
	fd = open(shadow, O_RDWR | O_CREAT | O_TRUNC, 0600);
	if (fd < 0) {
		fprintf(stderr, _("%s: Cannot open %s: %s\n"), Prog, shadow, strerror(errno));
		goto out_free;
	}
	close(fd);
	if (chown(shadow, 0, authgid)) {
		fprintf(stderr, _("%s: Cannot change owner of %s: %s\n"), Prog, shadow, strerror(errno));
		goto out_free;
	}
	if (chmod(shadow, authgid == shadowgid ? 0600 : 0640)) {
		fprintf(stderr, _("%s: Cannot change mode of %s: %s\n"), Prog, shadow, strerror(errno));
		goto out_free;
	}
	if (chown(dir, 0, authgid)) {
		fprintf(stderr, _("%s: Cannot change owner of %s: %s\n"), Prog, dir, strerror(errno));
		goto out_free;
	}
	if (chmod(dir, authgid == shadowgid ? 02700 : 02710)) {
		fprintf(stderr, _("%s: Cannot change mode of %s: %s\n"), Prog, dir, strerror(errno));
		goto out_free;
	}
	if (!shadowtcb_set_user(name) || !shadowtcb_move(NULL, uid))
		goto out_free;
	ret = 1;
out_free:
	free(dir);
	free(shadow);
	return ret;
}
