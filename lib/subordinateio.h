/*
 * Copyright (c) 2012- Eric W. Biederman
 */

#ifndef _SUBORDINATEIO_H
#define _SUBORDINATEIO_H

#include <config.h>

#ifdef ENABLE_SUBIDS

#include <sys/types.h>

extern int sub_uid_close(void);
extern bool is_sub_uid_range_free(uid_t start, unsigned long count);
extern bool have_sub_uids(const char *owner, uid_t start, unsigned long count);
extern bool sub_uid_file_present (void);
extern bool sub_uid_assigned(const char *owner);
extern int sub_uid_lock (void);
extern int sub_uid_setdbname (const char *filename);
extern /*@observer@*/const char *sub_uid_dbname (void);
extern int sub_uid_open (int mode);
extern int sub_uid_unlock (void);
extern int sub_uid_add (const char *owner, uid_t start, unsigned long count);
extern int sub_uid_remove (const char *owner, uid_t start, unsigned long count);
extern uid_t sub_uid_find_free_range(uid_t min, uid_t max, unsigned long count);

extern int sub_gid_close(void);
extern bool is_sub_gid_range_free(gid_t start, unsigned long count);
extern bool have_sub_gids(const char *owner, gid_t start, unsigned long count);
extern bool sub_gid_file_present (void);
extern bool sub_gid_assigned(const char *owner);
extern int sub_gid_lock (void);
extern int sub_gid_setdbname (const char *filename);
extern /*@observer@*/const char *sub_gid_dbname (void);
extern int sub_gid_open (int mode);
extern int sub_gid_unlock (void);
extern int sub_gid_add (const char *owner, gid_t start, unsigned long count);
extern int sub_gid_remove (const char *owner, gid_t start, unsigned long count);
extern uid_t sub_gid_find_free_range(gid_t min, gid_t max, unsigned long count);
#endif				/* ENABLE_SUBIDS */

#endif
