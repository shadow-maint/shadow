/*
 * Copyright (c) 1990 - 1994, Julianne Frances Haugh
 * Copyright (c) 1996 - 2000, Marek Michałkiewicz
 * Copyright (c) 2003 - 2006, Tomasz Kłoczko
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
#ifdef USE_UTMPX
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
#if defined (HAVE_SETGROUPS) && ! defined (USE_PAM)
extern int add_groups (const char *);
#endif

/* age.c */
extern void agecheck (/*@null@*/const struct spwd *);
extern int expire (const struct passwd *, /*@null@*/const struct spwd *);
/* isexpired.c */
extern int isexpired (const struct passwd *, /*@null@*/const struct spwd *);

/* basename() renamed to Basename() to avoid libc name space confusion */
/* basename.c */
extern char *Basename (char *str);

/* chowndir.c */
extern int chown_tree (const char *root,
                       uid_t old_uid, uid_t new_uid,
                       gid_t old_gid, gid_t new_gid);

/* chowntty.c */
extern void chown_tty (const struct passwd *);

/* cleanup.c */
typedef void (*cleanup_function) (/*@null@*/void *arg);
void add_cleanup (cleanup_function pcf, /*@null@*/void *arg);
void del_cleanup (cleanup_function pcf);
void do_cleanups (void);

/* cleanup_group.c */
struct cleanup_info_mod {
	char *audit_msg;
	char *action;
	char *name;
};
void cleanup_report_add_group (void *group_name);
void cleanup_report_add_group_group (void *group_name);
#ifdef SHADOWGRP
void cleanup_report_add_group_gshadow (void *group_name);
#endif
void cleanup_report_del_group (void *group_name);
void cleanup_report_del_group_group (void *group_name);
#ifdef SHADOWGRP
void cleanup_report_del_group_gshadow (void *group_name);
#endif
void cleanup_report_mod_passwd (void *cleanup_info);
void cleanup_report_mod_group (void *cleanup_info);
void cleanup_report_mod_gshadow (void *cleanup_info);
void cleanup_unlock_group (/*@null@*/void *unused);
#ifdef SHADOWGRP
void cleanup_unlock_gshadow (/*@null@*/void *unused);
#endif
void cleanup_unlock_passwd (/*@null@*/void *unused);

/* console.c */
extern bool console (const char *);

/* copydir.c */
extern int copy_tree (const char *src_root, const char *dst_root,
                      bool copy_root,
                      uid_t old_uid, uid_t new_uid,
                      gid_t old_gid, gid_t new_gid);
#ifdef WITH_SELINUX
extern int selinux_file_context (const char *dst_name);
#endif

/* encrypt.c */
extern char *pw_encrypt (const char *, const char *);

/* entry.c */
extern void pw_entry (const char *, struct passwd *);

/* env.c */
extern void addenv (const char *, /*@null@*/const char *);
extern void initenv (void);
extern void set_env (int, char *const *);
extern void sanitize_env (void);

/* fields.c */
extern void change_field (char *, size_t, const char *);
extern int valid_field (const char *, const char *);

/* find_new_gid.c */
extern int find_new_gid (bool sys_group,
                         gid_t *gid,
                         /*@null@*/gid_t const *preferred_gid);

/* find_new_uid.c */
extern int find_new_uid (bool sys_user,
                         uid_t *uid,
                         /*@null@*/uid_t const *preferred_uid);

/* get_gid.c */
extern int get_gid (const char *gidstr, gid_t *gid);

/* getgr_nam_gid.c */
extern /*@null@*/struct group *getgr_nam_gid (const char *grname);

/* getlong.c */
extern int getlong (const char *numstr, /*@out@*/long int *result);

/* get_pid.c */
extern int get_pid (const char *pidstr, pid_t *pid);

/* getrange */
extern int getrange (char *range,
                     unsigned long *min, bool *has_min,
                     unsigned long *max, bool *has_max);

/* get_uid.c */
extern int get_uid (const char *uidstr, uid_t *uid);

/* getulong.c */
extern int getulong (const char *numstr, /*@out@*/unsigned long int *result);

/* fputsx.c */
extern /*@null@*/char *fgetsx (/*@returned@*/ /*@out@*/char *, int, FILE *);
extern int fputsx (const char *, FILE *);

/* groupio.c */
extern void __gr_del_entry (const struct commonio_entry *ent);
extern /*@observer@*/const struct commonio_db *__gr_get_db (void);
extern /*@dependent@*/ /*@null@*/struct commonio_entry *__gr_get_head (void);
extern void __gr_set_changed (void);

/* groupmem.c */
extern /*@null@*/ /*@only@*/struct group *__gr_dup (const struct group *grent);
extern void gr_free (/*@out@*/ /*@only@*/struct group *grent);

/* hushed.c */
extern bool hushed (const char *username);

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
void audit_logger_message (const char *message, shadow_audit_result result);
#endif

/* limits.c */
#ifndef USE_PAM
extern void setup_limits (const struct passwd *);
#endif

/* list.c */
extern /*@only@*/ /*@out@*/char **add_list (/*@returned@*/ /*@only@*/char **, const char *);
extern /*@only@*/ /*@out@*/char **del_list (/*@returned@*/ /*@only@*/char **, const char *);
extern /*@only@*/ /*@out@*/char **dup_list (char *const *);
extern bool is_on_list (char *const *list, const char *member);
extern /*@only@*/char **comma_to_list (const char *);

/* log.c */
extern void dolastlog (
	struct lastlog *ll,
	const struct passwd *pw,
	/*@unique@*/const char *line,
	/*@unique@*/const char *host);

/* login_nopam.c */
extern int login_access (const char *user, const char *from);

/* loginprompt.c */
extern void login_prompt (const char *, char *, int);

/* mail.c */
extern void mailcheck (void);

/* motd.c */
extern void motd (void);

/* myname.c */
extern /*@null@*/struct passwd *get_my_pwent (void);

/* pam_pass_non_interractive.c */
#ifdef USE_PAM
extern int do_pam_passwd_non_interractive (const char *pam_service,
                                           const char *username,
                                           const char* password);
#endif				/* USE_PAM */

/* obscure.c */
#ifndef USE_PAM
extern int obscure (const char *, const char *, const struct passwd *);
#endif

/* pam_pass.c */
#ifdef USE_PAM
extern void do_pam_passwd (const char *user, bool silent, bool change_expired);
#endif

/* port.c */
extern bool isttytime (const char *, const char *, time_t);

/* pwd2spwd.c */
#ifndef USE_PAM
extern struct spwd *pwd_to_spwd (const struct passwd *);
#endif

/* pwdcheck.c */
#ifndef USE_PAM
extern void passwd_check (const char *, const char *, const char *);
#endif

/* pwd_init.c */
extern void pwd_init (void);

/* pwio.c */
extern void __pw_del_entry (const struct commonio_entry *ent);
extern struct commonio_db *__pw_get_db (void);
extern /*@dependent@*/ /*@null@*/struct commonio_entry *__pw_get_head (void);

/* pwmem.c */
extern /*@null@*/ /*@only@*/struct passwd *__pw_dup (const struct passwd *pwent);
extern void pw_free (/*@out@*/ /*@only@*/struct passwd *pwent);

/* remove_tree.c */
extern int remove_tree (const char *root, bool remove_root);

/* rlogin.c */
extern int do_rlogin (const char *remote_host, char *name, size_t namelen,
                      char *term, size_t termlen);

/* salt.c */
extern /*@observer@*/const char *crypt_make_salt (/*@null@*/const char *meth, /*@null@*/void *arg);

/* setugid.c */
extern int setup_groups (const struct passwd *info);
extern int change_uid (const struct passwd *info);
#if (defined HAVE_INITGROUPS) && (! defined USE_PAM)
extern int setup_uid_gid (const struct passwd *info, bool is_console);
#else
extern int setup_uid_gid (const struct passwd *info);
#endif

/* setup.c */
extern void setup (struct passwd *);

/* setupenv.c */
extern void setup_env (struct passwd *);

/* sgetgrent.c */
extern struct group *sgetgrent (const char *buf);

/* sgetpwent.c */
extern struct passwd *sgetpwent (const char *buf);

/* sgetspent.c */
#ifndef HAVE_SGETSPENT
extern struct spwd *sgetspent (const char *string);
#endif

/* sgroupio.c */
extern void __sgr_del_entry (const struct commonio_entry *ent);
extern /*@null@*/ /*@only@*/struct sgrp *__sgr_dup (const struct sgrp *sgent);
extern void sgr_free (/*@out@*/ /*@only@*/struct sgrp *sgent);
extern /*@dependent@*/ /*@null@*/struct commonio_entry *__sgr_get_head (void);
extern void __sgr_set_changed (void);

/* shadowio.c */
extern /*@dependent@*/ /*@null@*/struct commonio_entry *__spw_get_head (void);
extern void __spw_del_entry (const struct commonio_entry *ent);

/* shadowmem.c */
extern /*@null@*/ /*@only@*/struct spwd *__spw_dup (const struct spwd *spent);
extern void spw_free (/*@out@*/ /*@only@*/struct spwd *spent);

/* shell.c */
extern int shell (const char *file, /*@null@*/const char *arg, char *const envp[]);

/* system.c */
extern int safe_system (const char *command,
                        const char *argv[],
                        const char *env[],
                        int ignore_stderr);

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
#ifndef USE_PAM
extern char *tz (const char *);
#endif

/* ulimit.c */
extern int set_filesize_limit (int blocks);

/* user_busy.c */
extern int user_busy (const char *name, uid_t uid);

/* utmp.c */
extern /*@null@*/struct utmp *get_current_utmp (void);
extern struct utmp *prepare_utmp (const char *name,
                                  const char *line,
                                  const char *host,
                                  /*@null@*/const struct utmp *ut);
extern int setutmp (struct utmp *ut);
#ifdef USE_UTMPX
extern struct utmpx *prepare_utmpx (const char *name,
                                    const char *line,
                                    const char *host,
                                    /*@null@*/const struct utmp *ut);
extern int setutmpx (struct utmpx *utx);
#endif				/* USE_UTMPX */

/* valid.c */
extern bool valid (const char *, const struct passwd *);

/* xmalloc.c */
extern /*@maynotreturn@*/ /*@out@*//*@only@*/char *xmalloc (size_t size)
  /*@ensures MaxSet(result) == (size - 1); @*/;
extern /*@maynotreturn@*/ /*@only@*/char *xstrdup (const char *);

/* xgetpwnam.c */
extern /*@null@*/ /*@only@*/struct passwd *xgetpwnam (const char *);
/* xgetpwuid.c */
extern /*@null@*/ /*@only@*/struct passwd *xgetpwuid (uid_t);
/* xgetgrnam.c */
extern /*@null@*/ /*@only@*/struct group *xgetgrnam (const char *);
/* xgetgrgid.c */
extern /*@null@*/ /*@only@*/struct group *xgetgrgid (gid_t);
/* xgetspnam.c */
extern /*@null@*/ /*@only@*/struct spwd *xgetspnam(const char *);

/* yesno.c */
extern bool yes_or_no (bool read_only);

#endif				/* _PROTOTYPES_H */
