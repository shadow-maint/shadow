/* $Id: acconfig.h,v 1.12 1999/06/07 16:40:43 marekm Exp $ */



/* Define to enable password aging.  */
#undef AGING

/* Define if struct passwd has pw_age.  */
#undef ATT_AGE

/* Define if struct passwd has pw_comment.  */
#undef ATT_COMMENT

/* Define to support JFH's auth. methods.  UNTESTED.  */
#undef AUTH_METHODS

/* Define if struct passwd has pw_quota.  */
#undef BSD_QUOTA

/* Define if you have secure RPC.  */
#undef DES_RPC

/* Define to support 16-character passwords.  */
#undef DOUBLESIZE

/* Define to 1 if NLS is requested.  */
#undef ENABLE_NLS

/* Path for faillog file.  */
#undef FAILLOG_FILE

/* Define if you want my getgrent routines.  */
#undef GETGRENT

/* Define to libshadow_getpass to use our own version of getpass().  */
#undef getpass

/* Define if you want my getpwent routines.  */
#undef GETPWENT

/* Define as 1 if you have catgets and don't want to use GNU gettext.  */
#undef HAVE_CATGETS

/* Define as 1 if you have gettext and don't want to use GNU gettext.  */
#undef HAVE_GETTEXT

/* Define if your locale.h file contains LC_MESSAGES.  */
#undef HAVE_LC_MESSAGES

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

/* Define to 1 if you have the stpcpy function.  */
#undef HAVE_STPCPY

/* Define to support TCFS. */
#undef HAVE_TCFS

/* Path for lastlog file.  */
#undef LASTLOG_FILE

/* Define to support /etc/login.access login access control.  */
#undef LOGIN_ACCESS

/* Location of system mail spool directory.  */
#undef MAIL_SPOOL_DIR

/* Name of user's mail spool file if stored in user's home directory.  */
#undef MAIL_SPOOL_FILE

/* Define to support the MD5-based password hashing algorithm.  */
#undef MD5_CRYPT

/* Define to use ndbm.  */
#undef NDBM

/* Define to enable the new readpass() that echoes asterisks.  */
#undef NEW_READPASS

/* Define to support OPIE one-time password logins.  */
#undef OPIE

/* Package name.  */
#undef PACKAGE

/* Define if pam_strerror() needs two arguments (Linux-PAM 0.59).  */
#undef PAM_STRERROR_NEEDS_TWO_ARGS

/* Path to passwd program.  */
#undef PASSWD_PROGRAM

/* Define if the compiler understands function prototypes.  */
#undef PROTOTYPES

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

/* Define to support SecureWare(tm) long passwords.  */
#undef SW_CRYPT

/* Define if you want gdbm for TCFS. */
#undef TCFS_GDBM_SUPPORT

/* Define to support Pluggable Authentication Modules.  */
#undef USE_PAM

/* Define to use syslog().  */
#undef USE_SYSLOG

/* Define if you have ut_host in struct utmp.  */
#undef UT_HOST

/* Path for utmp file.  */
#undef _UTMP_FILE

/* Define to ut_name if struct utmp has ut_name (not ut_user).  */
#undef ut_user

/* Version.  */
#undef VERSION

/* Path for wtmp file.  */
#undef _WTMP_FILE

