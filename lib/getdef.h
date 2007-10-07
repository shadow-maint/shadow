#ifndef _GETDEF_H
#define _GETDEF_H

/* getdef.c */
extern int getdef_bool (const char *);
extern long getdef_long (const char *, long);
extern int getdef_num (const char *, int);
extern unsigned int getdef_unum (const char *, unsigned int);
extern char *getdef_str (const char *);
extern int putdef_str (const char *, const char *);

/* default UMASK value if not specified in /etc/login.defs */
#define		GETDEF_DEFAULT_UMASK	022

#endif				/* _GETDEF_H */
