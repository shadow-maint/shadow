#ifndef _GETDEF_H
#define _GETDEF_H

/* getdef.c */
extern int getdef_bool P_((const char *));
extern long getdef_long P_((const char *, long));
extern int getdef_num P_((const char *, int));
extern char *getdef_str P_((const char *));
extern int putdef_str P_((const char *, const char *));

#endif /* _GETDEF_H */
