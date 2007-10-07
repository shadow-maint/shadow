
#include <config.h>

#ifdef HAVE_TCFS

#include "prototypes.h"
#include "defines.h"

#ifdef TCFS_GDBM_SUPPORT
#undef GDBM_SUPPORT
#define GDBM_SUPPORT
#endif

#include <tcfslib.h>
#include <stdio.h>

#include "commonio.h"
#include "tcfsio.h"

static struct commonio_db tcfs_db = {
	TCFSPWDFILE,	/* filename */
	NULL,	/* ops */
	NULL,
	NULL,
	NULL,
	NULL,
	0,
	0,
	0,
	0,
	1
};

int
tcfs_file_present(void)
{
	return commonio_present(&tcfs_db);
}

int
tcfs_lock(void)
{
	return commonio_lock(&tcfs_db);
}

int
tcfs_open(int mode)
{
	return 1;
/*	return tcfs_open(); */
}

tcfspwdb *
tcfs_locate(char *name)
{
	return tcfs_getpwnam(name, NULL);
}

int
tcfs_update(char *user, struct tcfspwd *tcfspword)
{
	char *o, *p;
		
	o=(char*)calloc(128,sizeof(char));
	p=(char*)calloc(128,sizeof(char));
	strcpy (o, tcfspword->tcfsorig);
	strcpy (p, tcfspword->tcfspass);
	return tcfs_chgkey(user,o,p);
}

int
tcfs_remove(char *name)
{
	return tcfs_putpwnam(name, NULL, U_DEL);
}

int
tcfs_close(void)
{
	return 1;
/* return tcfs_close(&shadow_db); */
}

int
tcfs_unlock(void)
{
	return commonio_unlock(&tcfs_db);
}

#endif
