/*
 * prototypes.h
 *
 * Missing function prototypes
 *
 * Juha Virtanen, <jiivee@hut.fi>; November 1995
 */
/*
 * $Id: prototypes.h,v 1.13 1999/07/09 18:02:43 marekm Exp $
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
extern int add_groups P_((const char *));
extern void add_cons_grps P_((void));

/* age.c */
#ifdef SHADOWPWD
extern void agecheck P_((const struct passwd *pw, const struct spwd *sp));
extern int expire P_((const struct passwd *pw, const struct spwd *sp));
extern int isexpired P_((const struct passwd *pw, const struct spwd *sp));
#else
extern void agecheck P_((const struct passwd *pw));
extern int expire P_((const struct passwd *pw));
extern int isexpired P_((const struct passwd *pw));
#endif

/* basename() renamed to Basename() to avoid libc name space confusion */
/* basename.c */
extern char *Basename P_((char *str));

/* chkshell.c */
extern int check_shell P_((const char *));

/* chowndir.c */
extern int chown_tree P_((const char *, uid_t, uid_t, gid_t, gid_t));

/* chowntty.c */
extern void chown_tty P_((const char *, const struct passwd *));

/* console.c */
extern int console P_((const char *tty));
extern int is_listed P_((const char *cfgin, const char *tty, int def));

/* copydir.c */
extern int copy_tree P_((const char *, const char *, uid_t, gid_t));
extern int remove_tree P_((const char *));

/* encrypt.c */
extern char *pw_encrypt P_((const char *, const char *));

/* entry.c */
extern void entry P_((const char *name, struct passwd *pwent));

/* env.c */
extern void addenv P_((const char *, const char *));
extern void initenv P_((void));
extern void set_env P_((int, char * const *));
extern void sanitize_env P_((void));

/* fields.c */
extern void change_field P_((char *buf, size_t maxsize, const char *prompt));
extern int valid_field P_((const char *field, const char *illegal));

/* fputsx.c */
extern char *fgetsx P_((char *, int, FILE *));
extern int fputsx P_((const char *, FILE *));

/* grdbm.c */
extern int gr_dbm_remove P_((const struct group *gr));
extern int gr_dbm_update P_((const struct group *gr));
extern int gr_dbm_present P_((void));

/* grent.c */
extern int putgrent P_((const struct group *, FILE *));

/* grpack.c */
extern int gr_pack P_((const struct group *group, char *buf));
extern int gr_unpack P_((char *buf, int len, struct group *group));

#ifdef SHADOWGRP
/* gsdbm.c */
extern int sg_dbm_remove P_((const char *name));
extern int sg_dbm_update P_((const struct sgrp *sgr));
extern int sg_dbm_present P_((void));

/* gspack.c */
extern int sgr_pack P_((const struct sgrp *sgrp, char *buf));
extern int sgr_unpack P_((char *buf, int len, struct sgrp *sgrp));
#endif

/* hushed.c */
extern int hushed P_((const struct passwd *pw));

/* limits.c */
extern void setup_limits P_((const struct passwd *));

/* list.c */
extern char **add_list P_((char **list, const char *member));
extern char **del_list P_((char **list, const char *member));
extern char **dup_list P_((char * const *list));
extern int is_on_list P_((char * const *list, const char *member));
extern char **comma_to_list P_((const char *comma));

/* login.c */
extern void login_prompt P_((const char *, char *, int));

/* login_desrpc.c */
extern int login_desrpc P_((const char *));

/* mail.c */
extern void mailcheck P_((void));

/* motd.c */
extern void motd P_((void));

/* myname.c */
extern struct passwd *get_my_pwent P_((void));

/* obscure.c */
extern int obscure P_((const char *, const char *, const struct passwd *));

/* pam_pass.c */
extern int do_pam_passwd P_((const char *, int, int));

/* port.c */
extern int isttytime P_((const char *, const char *, time_t));

/* pwd2spwd.c */
#ifdef SHADOWPWD
extern struct spwd *pwd_to_spwd P_((const struct passwd *pw));
#endif

/* pwdcheck.c */
extern void passwd_check P_((const char *, const char *, const char *));

/* pwd_init.c */
extern void pwd_init P_((void));

/* pwdbm.c */
extern int pw_dbm_remove P_((const struct passwd *pw));
extern int pw_dbm_update P_((const struct passwd *pw));
extern int pw_dbm_present P_((void));

/* pwpack.c */
extern int pw_pack P_((const struct passwd *passwd, char *buf));
extern int pw_unpack P_((char *buf, int len, struct passwd *passwd));

/* rad64.c */
extern int c64i P_((char c));
extern int i64c P_((int i));

/* rlogin.c */
extern int do_rlogin P_((const char *, char *, int, char *, int));

/* setugid.c */
extern int setup_groups P_((const struct passwd *));
extern int change_uid P_((const struct passwd *));
extern int setup_uid_gid P_((const struct passwd *, int));

/* setup.c */
extern void setup P_((struct passwd *info));

/* setupenv.c */
extern void setup_env P_((struct passwd *));

/* shell.c */
extern void shell P_((const char *file, const char *arg));

#ifdef SHADOWPWD
/* spdbm.c */
extern int sp_dbm_remove P_((const char *user));
extern int sp_dbm_update P_((const struct spwd *sp));
extern int sp_dbm_present P_((void));

/* sppack.c */
extern int spw_pack P_((const struct spwd *spwd, char *buf));
extern int spw_unpack P_((char *buf, int len, struct spwd *spwd));
#endif

/* strtoday.c */
extern long strtoday P_((const char *str));

/* ttytype.c */
extern void ttytype P_((const char *line));

/* ulimit.c */
extern void set_filesize_limit P_((int));

/* utmp.c */
extern void checkutmp P_((int));
extern void setutmp P_((const char *, const char *, const char *));

/* valid.c */
extern int valid P_((const char *, const struct passwd *));

/* xmalloc.c */
extern char *xmalloc P_((size_t size));
extern char *xstrdup P_((const char *str));

#endif /* _PROTOTYPES_H */
