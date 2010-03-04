#ifndef _TCBFUNCS_H
#define _TCBFUNCS_H

#include <sys/types.h>

extern int shadowtcb_drop_priv();
extern int shadowtcb_gain_priv();
extern int shadowtcb_set_user(const char *name);
extern int shadowtcb_remove(const char *name);
extern int shadowtcb_move(const char *user_newname, uid_t user_newid);
extern int shadowtcb_create(const char *name, uid_t uid);

#endif
