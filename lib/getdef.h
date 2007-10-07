#ifndef _GETDEF_H
#define _GETDEF_H

/* getdef.c */
extern int getdef_bool (const char *);
extern long getdef_long (const char *, long);
extern int getdef_num (const char *, int);
extern unsigned int getdef_unum (const char *, unsigned int);
extern char *getdef_str (const char *);
extern int putdef_str (const char *, const char *);

#endif				/* _GETDEF_H */
