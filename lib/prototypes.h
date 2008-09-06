/*
 * Copyright (c) 1990 - 1994, Julianne Frances Haugh
 * Copyright (c) 1996 - 2000, Marek Michałkiewicz
 * Copyright (c) 2003 - 2006, Tomasz Kłoczko
 * Copyright (c) 2007 - 2008, Nicolas François
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

/*
 * prototypes.h
 *
 * prototypes of libmisc functions, and private lib functions.
 *
 * $Id$
 *
 */

#ifndef _PROTOTYPES_H
#define _PROTOTYPES_H

#include <sys/stat.h>
#if HAVE_UTMPX_H
#include <utmpx.h>
#else
#include <utmp.h>
#endif
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <shadow.h>
#include <lastlog.h>

#include "defines.h"
#include "commonio.h"

extern char *Prog;

/* addgrps.c */
extern int add_groups (const char *);
extern void add_cons_grps (void);

/* age.c */
extern void agecheck (const struct passwd *, const struct spwd *);
extern int expire (const struct passwd *, const struct spwd *);
extern int isexpired (const struct passwd *, const struct spwd *);

/* basename() renamed to Basename() to avoid libc name space confusion */
/* basename.c */
extern char *Basename (char *str);

/* chowndir.c */
extern int chown_tree (const char *, uid_t, uid_t, gid_t, gid_t);

/* chowntty.c */
extern void chown_tty (const char *, const struct passwd *);

/* console.c */
extern bool console (const char *);

/* copydir.c */
extern int copy_tree (const char *src_root, const char *dst_root,
                      long int uid, long int gid);
extern int remove_tree (const char *root);

/* encrypt.c */
extern char *pw_encrypt (const char *, const char *);

/* entry.c */
extern void pw_entry (const char *, struct passwd *);

/* env.c */
extern void addenv (const char *, const char *);
extern void initenv (void);
extern void set_env (int, char *const *);
extern void sanitize_env (void);

/* fields.c */
extern void change_field (char *, size_t, const char *);
extern int valid_field (const char *, const char *);

/* find_new_gid.c */
extern int find_new_gid (bool sys_group, gid_t *gid, gid_t const *preferred_gid);

/* find_new_uid.c */
extern int find_new_uid (bool sys_user, uid_t *uid, uid_t const *preferred_uid);

/* getlong.c */
extern int getlong(const char *numstr, long int *result);

/* getrange */
extern int getrange(char *range,
                    unsigned long *min, bool *has_min,
                    unsigned long *max, bool *has_max);

/* fputsx.c */
extern char *fgetsx (char *, int, FILE *);
extern int fputsx (const char *, FILE *);

/* groupio.c */
extern void __gr_del_entry (const struct commonio_entry *ent);
extern struct commonio_db *__gr_get_db (void);
extern struct commonio_entry *__gr_get_head (void);
extern void __gr_set_changed (void);

/* groupmem.c */
extern struct group *__gr_dup (const struct group *grent);

/* hushed.c */
extern bool hushed (const struct passwd *pw);

/* audit_help.c */
#ifdef WITH_AUDIT
extern int audit_fd;
extern void audit_help_open (void);
/* Use AUDIT_NO_ID when a name is provided to audit_logger instead of an ID */
#define AUDIT_NO_ID	((unsigned int) -1)
typedef enum {
	SHADOW_AUDIT_FAILURE = 0,
	SHADOW_AUDIT_SUCCESS = 1} shadow_audit_result;
extern void audit_logger (int type, const char *pgname, const char *op,
                          const char *name, unsigned int id,
                          shadow_audit_result result);
#endif

/* limits.c */
extern void setup_limits (const struct passwd *);

/* list.c */
extern char **add_list (char **, const char *);
extern char **del_list (char **, const char *);
extern char **dup_list (char *const *);
extern bool is_on_list (char *const *list, const char *member);
extern char **comma_to_list (const char *);

/* log.c */
extern void dolastlog (struct lastlog *ll,
                       const struct passwd *pw,
                       const char *line,
                       const char *host);

/* login_nopam.c */
extern int login_access (const char *user, const char *from);

/* loginprompt.c */
extern void login_prompt (const char *, char *, int);

/* mail.c */
extern void mailcheck (void);

/* motd.c */
extern void motd (void);

/* myname.c */
extern struct passwd *get_my_pwent (void);

/* obscure.c */
extern int obscure (const char *, const char *, const struct passwd *);

/* pam_pass.c */
extern void do_pam_passwd (const char *user, bool silent, bool change_expired);

/* port.c */
extern bool isttytime (const char *, const char *, time_t);

/* pwd2spwd.c */
extern struct spwd *pwd_to_spwd (const struct passwd *);

/* pwdcheck.c */
extern void passwd_check (const char *, const char *, const char *);

/* pwd_init.c */
extern void pwd_init (void);

/* pwio.c */
extern void __pw_del_entry (const struct commonio_entry *ent);
extern struct commonio_db *__pw_get_db (void);
extern struct commonio_entry *__pw_get_head (void);

/* pwmem.c */
extern struct passwd *__pw_dup (const struct passwd *pwent);

/* rlogin.c */
extern int do_rlogin (const char *remote_host, char *name, size_t namelen,
                      char *term, size_t termlen);

/* salt.c */
extern char *crypt_make_salt (const char *meth, void *arg);

/* setugid.c */
extern int setup_groups (const struct passwd *info);
extern int change_uid (const struct passwd *info);
extern int setup_uid_gid (const struct passwd *info, bool is_console);

/* setup.c */
extern void setup (struct passwd *);

/* setupenv.c */
extern void setup_env (struct passwd *);

/* sgetgrent.c */
extern struct group *sgetgrent (const char *buf);

/* sgetpwent.c */
extern struct passwd *sgetpwent (const char *buf);

/* sgroupio.c */
extern void __sgr_del_entry (const struct commonio_entry *ent);
extern struct sgrp *__sgr_dup (const struct sgrp *sgent);
extern struct commonio_entry *__sgr_get_head (void);
extern void __sgr_set_changed (void);

/* shadowio.c */
extern struct commonio_entry *__spw_get_head (void);
extern void __spw_del_entry (const struct commonio_entry *ent);

/* shadowmem.c */
extern struct spwd *__spw_dup (const struct spwd *spent);

/* shell.c */
extern int shell (const char *, const char *, char *const *);

/* strtoday.c */
extern long strtoday (const char *);

/* suauth.c */
extern int check_su_auth (const char *actual_id, const char *wanted_id);

/* sulog.c */
extern void sulog (const char *tty,
                   bool success,
                   const char *oldname,
                   const char *name);

/* sub.c */
extern void subsystem (const struct passwd *);

/* ttytype.c */
extern void ttytype (const char *);

/* tz.c */
extern char *tz (const char *);

/* ulimit.c */
extern int set_filesize_limit (int blocks);

/* utmp.c */
extern void checkutmp (bool picky);
extern int setutmp (const char *, const char *, const char *);

/* valid.c */
extern bool valid (const char *, const struct passwd *);

/* xmalloc.c */
extern char *xmalloc (size_t);
extern char *xstrdup (const char *);

/* xgetpwnam.c */
extern struct passwd *xgetpwnam (const char *);
/* xgetpwuid.c */
extern struct passwd *xgetpwuid (uid_t);
/* xgetgrnam.c */
extern struct group *xgetgrnam (const char *);
/* xgetgrgid.c */
extern struct group *xgetgrgid (gid_t);
/* xgetspnam.c */
extern struct spwd *xgetspnam(const char *);

/* yesno.c */
extern bool yes_or_no (bool read_only);

#endif				/* _PROTOTYPES_H */
