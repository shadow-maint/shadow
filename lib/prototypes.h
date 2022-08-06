/*
 * SPDX-FileCopyrightText: 1990 - 1994, Julianne Frances Haugh
 * SPDX-FileCopyrightText: 1996 - 2000, Marek Michałkiewicz
 * SPDX-FileCopyrightText: 2003 - 2006, Tomasz Kłoczko
 * SPDX-FileCopyrightText: 2007 - 2010, Nicolas François
 *
 * SPDX-License-Identifier: BSD-3-Clause
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

#include <config.h>

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

/* addgrps.c */
#if defined (HAVE_SETGROUPS) && ! defined (USE_PAM)
extern int add_groups (const char *);
#endif

/* age.c */
extern void agecheck (/*@null@*/const struct spwd *);
extern int expire (const struct passwd *, /*@null@*/const struct spwd *);
/* isexpired.c */
extern int isexpired (const struct passwd *, /*@null@*/const struct spwd *);

/* btrfs.c */
#ifdef WITH_BTRFS
extern int btrfs_create_subvolume(const char *path);
extern int btrfs_remove_subvolume(const char *path);
extern int btrfs_is_subvolume(const char *path);
extern int is_btrfs(const char *path);
#endif

/* basename() renamed to Basename() to avoid libc name space confusion */
/* basename.c */
extern /*@observer@*/const char *Basename (const char *str);

/* chowndir.c */
extern int chown_tree (const char *root,
                       uid_t old_uid, uid_t new_uid,
                       gid_t old_gid, gid_t new_gid);

/* chowntty.c */
extern void chown_tty (const struct passwd *);

/* cleanup.c */
typedef /*@null@*/void (*cleanup_function) (/*@null@*/void *arg);
void add_cleanup (/*@notnull@*/cleanup_function pcf, /*@null@*/void *arg);
void del_cleanup (/*@notnull@*/cleanup_function pcf);
void do_cleanups (void);

/* cleanup_group.c */
struct cleanup_info_mod {
	char *audit_msg;
	char *action;
	/*@observer@*/const char *name;
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
                      bool reset_selinux,
                      uid_t old_uid, uid_t new_uid,
                      gid_t old_gid, gid_t new_gid);

/* date_to_str.c */
extern void date_to_str (size_t size, char buf[size], long date);

/* encrypt.c */
extern /*@exposed@*//*@null@*/char *pw_encrypt (const char *, const char *);

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

#ifdef ENABLE_SUBIDS
/* find_new_sub_gids.c */
extern int find_new_sub_gids (gid_t *range_start, unsigned long *range_count);

/* find_new_sub_uids.c */
extern int find_new_sub_uids (uid_t *range_start, unsigned long *range_count);
#endif				/* ENABLE_SUBIDS */


/* get_gid.c */
extern int get_gid (const char *gidstr, gid_t *gid);

/* getgr_nam_gid.c */
extern /*@only@*//*@null@*/struct group *getgr_nam_gid (/*@null@*/const char *grname);

/* getlong.c */
extern int getlong (const char *numstr, /*@out@*/long int *result);

/* get_pid.c */
extern int get_pid (const char *pidstr, pid_t *pid);

/* getrange */
extern int getrange (const char *range,
                     unsigned long *min, bool *has_min,
                     unsigned long *max, bool *has_max);

/* gettime.c */
extern time_t gettime (void);

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
extern void gr_free_members (struct group *grent);
extern void gr_free (/*@out@*/ /*@only@*/struct group *grent);
extern bool gr_append_member (struct group *grp, char *member);

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
extern /*@null@*//*@only@*/struct passwd *get_my_pwent (void);

/* nss.c */
#include <libsubid/subid.h>
extern void nss_init(const char *nsswitch_path);
extern bool nss_is_initialized(void);

struct subid_nss_ops {
	/*
	 * nss_has_range: does a user own a given subid range
	 *
	 * @owner: username
	 * @start: first subid in queried range
	 * @count: number of subids in queried range
	 * @idtype: subuid or subgid
	 * @result: true if @owner has been allocated the subid range.
	 *
	 * returns success if the module was able to determine an answer (true or false),
	 * else an error status.
	 */
	enum subid_status (*has_range)(const char *owner, unsigned long start, unsigned long count, enum subid_type idtype, bool *result);

	/*
	 * nss_list_owner_ranges: list the subid ranges delegated to a user.
	 *
	 * @owner - string representing username being queried
	 * @id_type - subuid or subgid
	 * @ranges - pointer to an array of struct subid_range, or NULL.  The
	 *           returned array must be freed by the caller.
	 * @count - pointer to an integer into which the number of returned ranges
	 *          is written.

	 * returns success if the module was able to determine an answer,
	 * else an error status.
	 */
	enum subid_status (*list_owner_ranges)(const char *owner, enum subid_type id_type, struct subid_range **ranges, int *count);

	/*
	 * nss_find_subid_owners: find uids who own a given subuid or subgid.
	 *
	 * @id - the delegated id (subuid or subgid) being queried
	 * @id_type - subuid or subgid
	 * @uids - pointer to an array of uids which will be allocated by
	 *         nss_find_subid_owners()
	 * @count - number of uids found
	 *
	 * returns success if the module was able to determine an answer,
	 * else an error status.
	 */
	enum subid_status (*find_subid_owners)(unsigned long id, enum subid_type id_type, uid_t **uids, int *count);

	/* The dlsym handle to close */
	void *handle;
};

extern struct subid_nss_ops *get_subid_nss_handle(void);


/* pam_pass_non_interactive.c */
#ifdef USE_PAM
extern int do_pam_passwd_non_interactive (const char *pam_service,
                                           const char *username,
                                           const char* password);
#endif				/* USE_PAM */

/* obscure.c */
#ifndef USE_PAM
extern bool obscure (const char *, const char *, const struct passwd *);
#endif

/* pam_pass.c */
#ifdef USE_PAM
extern void do_pam_passwd (const char *user, bool silent, bool change_expired);
#endif

/* port.c */
extern bool isttytime (const char *, const char *, time_t);

/* prefix_flag.c */
extern const char* process_prefix_flag (const char* short_opt, int argc, char **argv);
extern struct group *prefix_getgrnam(const char *name);
extern struct group *prefix_getgrgid(gid_t gid);
extern struct passwd *prefix_getpwuid(uid_t uid);
extern struct passwd *prefix_getpwnam(const char* name);
extern struct spwd *prefix_getspnam(const char* name);
extern struct group *prefix_getgr_nam_gid(const char *grname);
extern void prefix_setpwent(void);
extern struct passwd* prefix_getpwent(void);
extern void prefix_endpwent(void);
extern void prefix_setgrent(void);
extern struct group* prefix_getgrent(void);
extern void prefix_endgrent(void);

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

/* root_flag.c */
extern void process_root_flag (const char* short_opt, int argc, char **argv);

/* salt.c */
extern /*@observer@*/const char *crypt_make_salt (/*@null@*//*@observer@*/const char *meth, /*@null@*/void *arg);

/* selinux.c */
#ifdef WITH_SELINUX
extern int set_selinux_file_context (const char *dst_name, mode_t mode);
extern void reset_selinux_handle (void);
extern int reset_selinux_file_context (void);
extern int check_selinux_permit (const char *perm_name);
#endif

/* semanage.c */
#ifdef WITH_SELINUX
extern int set_seuser(const char *login_name, const char *seuser_name);
extern int del_seuser(const char *login_name);
#endif

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

/* spawn.c */
extern int run_command (const char *cmd, const char *argv[],
                        /*@null@*/const char *envp[], /*@out@*/int *status);

/* strtoday.c */
extern long strtoday (const char *);

/* suauth.c */
extern int check_su_auth (const char *actual_id,
                          const char *wanted_id,
                          bool su_to_root);

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
extern /*@observer@*/const char *tz (const char *);
#endif

/* ulimit.c */
extern int set_filesize_limit (int blocks);

/* user_busy.c */
extern int user_busy (const char *name, uid_t uid);

/* utmp.c */
#ifndef USE_UTMPX
extern /*@null@*/struct utmp *get_current_utmp (void);
extern struct utmp *prepare_utmp (const char *name,
                                  const char *line,
                                  const char *host,
                                  /*@null@*/const struct utmp *ut);
extern int setutmp (struct utmp *ut);
#else
extern /*@null@*/struct utmpx *get_current_utmp (void);
extern struct utmpx *prepare_utmpx (const char *name,
                                    const char *line,
                                    const char *host,
                                    /*@null@*/const struct utmpx *ut);
extern int setutmpx (struct utmpx *utx);
#endif				/* USE_UTMPX */

/* valid.c */
extern bool valid (const char *, const struct passwd *);

/* xmalloc.c */
extern /*@maynotreturn@*/ /*@only@*//*@out@*//*@notnull@*/void *xmalloc (size_t size)
  /*@ensures MaxSet(result) == (size - 1); @*/;
extern /*@maynotreturn@*/ /*@only@*//*@notnull@*/char *xstrdup (const char *);
extern void xfree(void *ap);

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
