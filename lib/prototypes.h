/*
 * prototypes.h
 *
 * Missing function prototypes
 *
 * Juha Virtanen, <jiivee@hut.fi>; November 1995
 */
/*
 * $Id: prototypes.h,v 1.24 2005/09/05 16:22:03 kloczek Exp $
 *
 * Added a macro to work around ancient (non-ANSI) compilers, just in case
 * someone ever tries to compile this with SunOS cc...  --marekm
 */

#ifndef _PROTOTYPES_H
#define _PROTOTYPES_H

#include <sys/stat.h>
#if HAVE_UTMPX_H
#include <utmpx.h>
#else
#include <utmp.h>
#endif
#include <pwd.h>
#include <grp.h>

#include "defines.h"

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

/* chkshell.c */
extern int check_shell (const char *);

/* chowndir.c */
extern int chown_tree (const char *, uid_t, uid_t, gid_t, gid_t);

/* chowntty.c */
extern void chown_tty (const char *, const struct passwd *);

/* console.c */
extern int console (const char *);
extern int is_listed (const char *, const char *, int);

/* copydir.c */
extern int copy_tree (const char *, const char *, uid_t, gid_t);
extern int remove_tree (const char *);

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

/* fputsx.c */
extern char *fgetsx (char *, int, FILE *);
extern int fputsx (const char *, FILE *);

/* grdbm.c */
extern int gr_dbm_remove (const struct group *);
extern int gr_dbm_update (const struct group *);
extern int gr_dbm_present (void);

/* grent.c */
extern int putgrent (const struct group *, FILE *);

/* hushed.c */
extern int hushed (const struct passwd *);

/* audit_help.c */
#ifdef WITH_AUDIT
extern int audit_fd;
#endif
void audit_help_open (void);
void audit_help_log (const char *, ...)
#ifdef __GNUC__
    __attribute__ ((format (printf, 1, 2)));
#else
;
#endif

/* limits.c */
extern void setup_limits (const struct passwd *);

/* list.c */
extern char **add_list (char **, const char *);
extern char **del_list (char **, const char *);
extern char **dup_list (char *const *);
extern int is_on_list (char *const *, const char *);
extern char **comma_to_list (const char *);

/* login.c */
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
extern int do_pam_passwd (const char *, int, int);

/* port.c */
extern int isttytime (const char *, const char *, time_t);

/* pwd2spwd.c */
extern struct spwd *pwd_to_spwd (const struct passwd *);

/* pwdcheck.c */
extern void passwd_check (const char *, const char *, const char *);

/* pwd_init.c */
extern void pwd_init (void);

/* rlogin.c */
extern int do_rlogin (const char *, char *, int, char *, int);

/* salt.c */
extern char *crypt_make_salt (void);

/* setugid.c */
extern int setup_groups (const struct passwd *);
extern int change_uid (const struct passwd *);
extern int setup_uid_gid (const struct passwd *, int);

/* setup.c */
extern void setup (struct passwd *);

/* setupenv.c */
extern void setup_env (struct passwd *);

/* shell.c */
extern void shell (const char *, const char *);

/* strtoday.c */
extern long strtoday (const char *);

/* suauth.c */
extern int check_su_auth (const char *, const char *);

/* sulog.c */
extern void sulog (const char *, int, const char *, const char *);

/* sub.c */
extern void subsystem (const struct passwd *);

/* ttytype.c */
extern void ttytype (const char *);

/* tz.c */
extern char *tz (const char *);

/* ulimit.c */
extern void set_filesize_limit (int);

/* utmp.c */
extern void checkutmp (int);
extern void setutmp (const char *, const char *, const char *);

/* valid.c */
extern int valid (const char *, const struct passwd *);

/* xmalloc.c */
extern char *xmalloc (size_t);
extern char *xstrdup (const char *);

#endif				/* _PROTOTYPES_H */
