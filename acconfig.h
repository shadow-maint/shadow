/* $Id: acconfig.h,v 1.21 2003/12/17 01:46:59 kloczek Exp $ */



/* Define if you have secure RPC.  */
#undef DES_RPC

/* Path for faillog file.  */
#undef FAILLOG_FILE

/* Defined if you have libcrack.  */
#undef HAVE_LIBCRACK

/* Defined if you have the ts&szs cracklib.  */
#undef HAVE_LIBCRACK_HIST

/* Defined if it includes *Pw functions.  */
#undef HAVE_LIBCRACK_PW

/* Defined if you have libcrypt.  */
#undef HAVE_LIBCRYPT

/* Define if struct lastlog has ll_host */
#undef HAVE_LL_HOST

/* Working shadow group support in libc?  */
#undef HAVE_SHADOWGRP

/* Path for lastlog file.  */
#undef LASTLOG_FILE

/* Define to support /etc/login.access login access control.  */
#undef LOGIN_ACCESS

/* Location of system mail spool directory.  */
#undef MAIL_SPOOL_DIR

/* Name of user's mail spool file if stored in user's home directory.  */
#undef MAIL_SPOOL_FILE

/* Define to support OPIE one-time password logins.  */
#undef OPIE

/* Path to passwd program.  */
#undef PASSWD_PROGRAM

/* Define if login should support the -r flag for rlogind.  */
#undef RLOGIN

/* Define to the ruserok() "success" return value (0 or 1).  */
#undef RUSEROK

/* Define to support the shadow group file.  */
#undef SHADOWGRP

/* Define to support the shadow password file.  */
#undef SHADOWPWD

/* Define to support S/Key logins.  */
#undef SKEY

/* Define to support /etc/suauth su access control.  */
#undef SU_ACCESS

/* Define to use syslog().  */
#undef USE_SYSLOG

/* Define to support USG (Unix Systems Group?) behavior. */
#undef USG

/* Define if you have ut_host in struct utmp.  */
#undef UT_HOST

/* Path for utmp file.  */
#undef _UTMP_FILE

/* Path for wtmp file.  */
#undef _WTMP_FILE

/* Define to libshadow_getpass to use our own version of getpass().  */
#undef getpass

/* Define to ut_name if struct utmp has ut_name (not ut_user).  */
#undef ut_user
