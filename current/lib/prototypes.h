/*
 * prototypes.h
 *
 * Missing function prototypes
 *
 * Juha Virtanen, <jiivee@hut.fi>; November 1995
 */
/*
 * $Id: prototypes.h,v 1.14 2000/08/26 18:27:17 marekm Exp $
 *
 * Added a macro to work around ancient (non-ANSI) compilers, just in case
 * someone ever tries to compile this with SunOS cc...  --marekm
 */

#ifndef _PROTOTYPES_H
#define _PROTOTYPES_H

#include <sys/stat.h>
#include <utmp.h>
#include <pwd.h>
#include <grp.h>

#include "defines.h"

/* addgrps.c */
extern int add_groups(const char *);
extern void add_cons_grps(void);

/* age.c */
#ifdef SHADOWPWD
extern void agecheck(const struct passwd *, const struct spwd *);
extern int expire(const struct passwd *, const struct spwd *);
extern int isexpired(const struct passwd *, const struct spwd *);
#else
extern void agecheck(const struct passwd *);
extern int expire(const struct passwd *);
extern int isexpired(const struct passwd *);
#endif

/* basename() renamed to Basename() to avoid libc name space confusion */
/* basename.c */
extern char *Basename(char *str);

/* chkshell.c */
extern int check_shell(const char *);

/* chowndir.c */
extern int chown_tree(const char *, uid_t, uid_t, gid_t, gid_t);

/* chowntty.c */
extern void chown_tty(const char *, const struct passwd *);

/* console.c */
extern int console(const char *);
extern int is_listed(const char *, const char *, int);

/* copydir.c */
extern int copy_tree(const char *, const char *, uid_t, gid_t);
extern int remove_tree(const char *);

/* encrypt.c */
extern char *pw_encrypt(const char *, const char *);

/* entry.c */
extern void pw_entry(const char *, struct passwd *);

/* env.c */
extern void addenv(const char *, const char *);
extern void initenv(void);
extern void set_env(int, char * const *);
extern void sanitize_env(void);

/* fields.c */
extern void change_field(char *, size_t, const char *);
extern int valid_field(const char *, const char *);

/* fputsx.c */
extern char *fgetsx(char *, int, FILE *);
extern int fputsx(const char *, FILE *);

/* grdbm.c */
extern int gr_dbm_remove(const struct group *);
extern int gr_dbm_update(const struct group *);
extern int gr_dbm_present(void);

/* grent.c */
extern int putgrent(const struct group *, FILE *);

/* grpack.c */
extern int gr_pack(const struct group *, char *);
extern int gr_unpack(char *, int, struct group *);

#ifdef SHADOWGRP
/* gsdbm.c */
extern int sg_dbm_remove(const char *);
extern int sg_dbm_update(const struct sgrp *);
extern int sg_dbm_present(void);

/* gspack.c */
extern int sgr_pack(const struct sgrp *, char *);
extern int sgr_unpack(char *, int, struct sgrp *);
#endif

/* hushed.c */
extern int hushed(const struct passwd *);

/* limits.c */
extern void setup_limits(const struct passwd *);

/* list.c */
extern char **add_list(char **, const char *);
extern char **del_list(char **, const char *);
extern char **dup_list(char * const *);
extern int is_on_list(char * const *, const char *);
extern char **comma_to_list(const char *);

/* login.c */
extern void login_prompt(const char *, char *, int);

/* login_desrpc.c */
extern int login_desrpc(const char *);

/* mail.c */
extern void mailcheck(void);

/* motd.c */
extern void motd(void);

/* myname.c */
extern struct passwd *get_my_pwent(void);

/* obscure.c */
extern int obscure(const char *, const char *, const struct passwd *);

/* pam_pass.c */
extern int do_pam_passwd(const char *, int, int);

/* port.c */
extern int isttytime(const char *, const char *, time_t);

/* pwd2spwd.c */
#ifdef SHADOWPWD
extern struct spwd *pwd_to_spwd(const struct passwd *);
#endif

/* pwdcheck.c */
extern void passwd_check(const char *, const char *, const char *);

/* pwd_init.c */
extern void pwd_init(void);

/* pwdbm.c */
extern int pw_dbm_remove(const struct passwd *);
extern int pw_dbm_update(const struct passwd *);
extern int pw_dbm_present(void);

/* pwpack.c */
extern int pw_pack(const struct passwd *, char *);
extern int pw_unpack(char *, int, struct passwd *);

/* rad64.c */
extern int c64i(int);
extern int i64c(int);

/* rlogin.c */
extern int do_rlogin(const char *, char *, int, char *, int);

/* salt.c */
extern char *crypt_make_salt(void);

/* setugid.c */
extern int setup_groups(const struct passwd *);
extern int change_uid(const struct passwd *);
extern int setup_uid_gid(const struct passwd *, int);

/* setup.c */
extern void setup(struct passwd *);

/* setupenv.c */
extern void setup_env(struct passwd *);

/* shell.c */
extern void shell(const char *, const char *);

#ifdef SHADOWPWD
/* spdbm.c */
extern int sp_dbm_remove(const char *);
extern int sp_dbm_update(const struct spwd *);
extern int sp_dbm_present(void);

/* sppack.c */
extern int spw_pack(const struct spwd *, char *);
extern int spw_unpack(char *, int, struct spwd *);
#endif

/* strtoday.c */
extern long strtoday(const char *);

/* suauth.c */
extern int check_su_auth(const char *, const char *);

/* sulog.c */
extern void sulog(const char *, int, const char *, const char *);

/* sub.c */
extern void subsystem(const struct passwd *);

/* ttytype.c */
extern void ttytype(const char *);

/* tz.c */
extern char *tz(const char *);

/* ulimit.c */
extern void set_filesize_limit(int);

/* utmp.c */
extern void checkutmp(int);
extern void setutmp(const char *, const char *, const char *);

/* valid.c */
extern int valid(const char *, const struct passwd *);

/* xmalloc.c */
extern char *xmalloc(size_t);
extern char *xstrdup(const char *);

#endif /* _PROTOTYPES_H */
