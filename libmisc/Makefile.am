
EXTRA_DIST = .indent.pro xgetXXbyYY.c

AM_CPPFLAGS = -I$(top_srcdir)/lib -I$(top_srcdir) $(ECONF_CPPFLAGS)

noinst_LTLIBRARIES = libmisc.la

if USE_PAM
LIBCRYPT_PAM = $(LIBCRYPT)
else
LIBCRYPT_PAM =
endif

libmisc_la_CFLAGS = $(LIBBSD_CFLAGS) $(LIBCRYPT_PAM) $(LIBSYSTEMD)
libmisc_la_SOURCES = \
	addgrps.c \
	age.c \
	agetpass.c \
	alloc.c \
	audit_help.c \
	basename.c \
	bit.c \
	chkname.c \
	chkname.h \
	chowndir.c \
	chowntty.c \
	cleanup.c \
	cleanup_group.c \
	cleanup_user.c \
	console.c \
	copydir.c \
	date_to_str.c \
	entry.c \
	env.c \
	failure.c \
	failure.h \
	find_new_gid.c \
	find_new_uid.c \
	find_new_sub_gids.c \
	find_new_sub_uids.c \
	getdate.h \
	getdate.y \
	getgr_nam_gid.c \
	getrange.c \
	gettime.c \
	hushed.c \
	idmapping.h \
	idmapping.c \
	isexpired.c \
	limits.c \
	list.c \
	loginprompt.c \
	mail.c \
	mempcpy.c \
	motd.c \
	myname.c \
	obscure.c \
	pam_pass.c \
	pam_pass_non_interactive.c \
	prefix_flag.c \
	pwd2spwd.c \
	pwdcheck.c \
	pwd_init.c \
	csrand.c \
	remove_tree.c \
	rlogin.c \
	root_flag.c \
	salt.c \
	setugid.c \
	setupenv.c \
	shell.c \
	stpecpy.c \
	stpeprintf.c \
	strtoday.c \
	sub.c \
	sulog.c \
	ttytype.c \
	tz.c \
	ulimit.c \
	user_busy.c \
	valid.c \
	xgetpwnam.c \
	xprefix_getpwnam.c \
	xgetpwuid.c \
	xgetgrnam.c \
	xgetgrgid.c \
	xgetspnam.c \
	yesno.c

if WITH_BTRFS
libmisc_la_SOURCES += btrfs.c
endif

if ENABLE_LASTLOG
libmisc_la_SOURCES += log.c
endif

if ENABLE_LOGIND
libmisc_la_SOURCES += logind.c
else
libmisc_la_SOURCES += utmp.c
endif
