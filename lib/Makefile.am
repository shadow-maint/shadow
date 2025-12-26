
AUTOMAKE_OPTIONS = 1.0 foreign

DEFS =

noinst_LTLIBRARIES = libshadow.la

if USE_PAM
LIBCRYPT_PAM = $(LIBCRYPT)
else
LIBCRYPT_PAM =
endif

AM_CPPFLAGS = -I$(top_srcdir)/lib -I$(top_srcdir) $(ECONF_CPPFLAGS)

libshadow_la_CPPFLAGS = $(ECONF_CPPFLAGS)
if HAVE_VENDORDIR
libshadow_la_CPPFLAGS += -DVENDORDIR=\"$(VENDORDIR)\"
endif

libshadow_la_CPPFLAGS += -I$(top_srcdir)
libshadow_la_CFLAGS = $(LIBBSD_CFLAGS) $(LIBCRYPT_PAM) $(LIBSYSTEMD)
libshadow_la_LIBADD = $(LIBADD_DLOPEN)

libshadow_la_SOURCES = \
	addgrps.c \
	adds.c \
	adds.h \
	age.c \
	agetpass.c \
	agetpass.h \
	alloc/calloc.c \
	alloc/calloc.h \
	alloc/malloc.c \
	alloc/malloc.h \
	alloc/realloc.c \
	alloc/realloc.h \
	alloc/reallocf.c \
	alloc/reallocf.h \
	atoi/a2i.c \
	atoi/a2i.h \
	atoi/getnum.c \
	atoi/getnum.h \
	atoi/strtoi/strtoi.c \
	atoi/strtoi/strtoi.h \
	atoi/strtoi/strtou.c \
	atoi/strtoi/strtou.h \
	atoi/strtoi/strtou_noneg.c \
	atoi/strtoi/strtou_noneg.h \
	attr.h \
	audit_help.c \
	basename.c \
	bit.c \
	bit.h \
	cast.h \
	chkname.c \
	chkname.h \
	chkhash.c \
	chkhash.h \
	chowndir.c \
	chowntty.c \
	cleanup.c \
	cleanup_group.c \
	cleanup_user.c \
	commonio.c \
	commonio.h \
	console.c \
	copydir.c \
	csrand.c \
	defines.h \
	encrypt.c \
	env.c \
	exit_if_null.c \
	exit_if_null.h \
	exitcodes.h \
	faillog.h \
	failure.c \
	failure.h \
	fd.c \
	fields.c \
	fields.h \
	find_new_gid.c \
	find_new_uid.c \
	find_new_sub_gids.c \
	find_new_sub_uids.c \
	fs/mkstemp/fmkomstemp.c \
	fs/mkstemp/fmkomstemp.h \
	fs/mkstemp/mkomstemp.c \
	fs/mkstemp/mkomstemp.h \
	fs/readlink/areadlink.c \
	fs/readlink/areadlink.h \
	fs/readlink/readlinknul.c \
	fs/readlink/readlinknul.h \
	get_pid.c \
	getdef.c \
	getdef.h \
	getgr_nam_gid.c \
	getrange.c \
	gettime.c \
	groupio.c \
	groupmem.c \
	groupio.h \
	hushed.c \
	idmapping.h \
	idmapping.c \
	io/fprintf.c \
	io/fprintf.h \
	io/syslog.c \
	io/syslog.h \
	isexpired.c \
	limits.c \
	list.c \
	lockpw.c \
	loginprompt.c \
	mail.c \
	motd.c \
	myname.c \
	nss.c \
	nscd.c \
	nscd.h \
	obscure.c \
	pam_defs.h \
	pam_pass.c \
	pam_pass_non_interactive.c \
	port.c \
	port.h \
	prefix_flag.c \
	prototypes.h \
	pwauth.c \
	pwauth.h \
	pwio.c \
	pwio.h \
	pwd_init.c \
	pwd2spwd.c \
	pwdcheck.c \
	pwmem.c \
	remove_tree.c \
	root_flag.c \
	run_part.h \
	run_part.c \
	salt.c \
	search/cmp/cmp.c \
	search/cmp/cmp.h \
	search/l/lfind.c \
	search/l/lfind.h \
	search/l/lsearch.c \
	search/l/lsearch.h \
	search/sort/qsort.c \
	search/sort/qsort.h \
	selinux.c \
	semanage.c \
	setugid.c \
	setupenv.c \
	sgroupio.c \
	sgroupio.h \
	shadow/group/sgetgrent.c \
	shadow/group/sgetgrent.h \
	shadow/grp/agetgroups.c \
	shadow/grp/agetgroups.h \
	shadow/gshadow/endsgent.c \
	shadow/gshadow/endsgent.h \
	shadow/gshadow/fgetsgent.c \
	shadow/gshadow/fgetsgent.h \
	shadow/gshadow/getsgent.c \
	shadow/gshadow/getsgent.h \
	shadow/gshadow/getsgnam.c \
	shadow/gshadow/getsgnam.h \
	shadow/gshadow/gshadow.c \
	shadow/gshadow/gshadow.h \
	shadow/gshadow/putsgent.c \
	shadow/gshadow/putsgent.h \
	shadow/gshadow/setsgent.c \
	shadow/gshadow/setsgent.h \
	shadow/gshadow/sgetsgent.c \
	shadow/gshadow/sgetsgent.h \
	shadow/gshadow/sgrp.c \
	shadow/gshadow/sgrp.h \
	shadow/passwd/sgetpwent.c \
	shadow/passwd/sgetpwent.h \
	shadow/shadow/sgetspent.c \
	shadow/shadow/sgetspent.h \
	shadowio.c \
	shadowio.h \
	shadowlog.c \
	shadowlog.h \
	shadowlog_internal.h \
	shadowmem.c \
	shell.c \
	sizeof.h \
	spawn.c \
	sssd.c \
	sssd.h \
	string/ctype/strchrisascii/strchriscntrl.c \
	string/ctype/strchrisascii/strchriscntrl.h \
	string/ctype/strisascii/strisdigit.c \
	string/ctype/strisascii/strisdigit.h \
	string/ctype/strisascii/strisprint.c \
	string/ctype/strisascii/strisprint.h \
	string/ctype/strtoascii/strtolower.c \
	string/ctype/strtoascii/strtolower.h \
	string/memset/memzero.c \
	string/memset/memzero.h \
	string/sprintf/aprintf.c \
	string/sprintf/aprintf.h \
	string/sprintf/snprintf.c \
	string/sprintf/snprintf.h \
	string/sprintf/stpeprintf.c \
	string/sprintf/stpeprintf.h \
	string/strchr/strchrcnt.c \
	string/strchr/strchrcnt.h \
	string/strchr/strchrscnt.c \
	string/strchr/strchrscnt.h \
	string/strchr/strnul.c \
	string/strchr/strnul.h \
	string/strcmp/strcaseeq.c \
	string/strcmp/strcaseeq.h \
	string/strcmp/strcaseprefix.c \
	string/strcmp/strcaseprefix.h \
	string/strcmp/streq.c \
	string/strcmp/streq.h \
	string/strcmp/strneq.c \
	string/strcmp/strneq.h \
	string/strcmp/strprefix.c \
	string/strcmp/strprefix.h \
	string/strcpy/stpecpy.c \
	string/strcpy/stpecpy.h \
	string/strcpy/strncat.c \
	string/strcpy/strncat.h \
	string/strcpy/strncpy.c \
	string/strcpy/strncpy.h \
	string/strcpy/strtcpy.c \
	string/strcpy/strtcpy.h \
	string/strdup/strdup.c \
	string/strdup/strdup.h \
	string/strdup/strndupa.c \
	string/strdup/strndupa.h \
	string/strdup/strndup.c \
	string/strdup/strndup.h \
	string/strerrno.c \
	string/strerrno.h \
	string/strftime.c \
	string/strftime.h \
	string/strspn/stpspn.c \
	string/strspn/stpspn.h \
	string/strspn/stprcspn.c \
	string/strspn/stprcspn.h \
	string/strspn/stprspn.c \
	string/strspn/stprspn.h \
	string/strspn/strrcspn.c \
	string/strspn/strrcspn.h \
	string/strspn/strrspn.c \
	string/strspn/strrspn.h \
	string/strtok/stpsep.c \
	string/strtok/stpsep.h \
	string/strtok/astrsep2ls.c \
	string/strtok/astrsep2ls.h \
	string/strtok/strsep2arr.c \
	string/strtok/strsep2arr.h \
	string/strtok/strsep2ls.c \
	string/strtok/strsep2ls.h \
	strtoday.c \
	sub.c \
	subordinateio.h \
	subordinateio.c \
	sulog.c \
	time/day_to_str.c \
	time/day_to_str.h \
	ttytype.c \
	typetraits.h \
	tz.c \
	ulimit.c \
	user_busy.c \
	valid.c \
	write_full.c \
	xgetpwnam.c \
	xprefix_getpwnam.c \
	xgetpwuid.c \
	xgetgrnam.c \
	xgetgrgid.c \
	xgetspnam.c \
	yesno.c

if WITH_TCB
libshadow_la_SOURCES += tcbfuncs.c tcbfuncs.h
endif

if WITH_BTRFS
libshadow_la_SOURCES += btrfs.c
endif

if ENABLE_LASTLOG
libshadow_la_SOURCES += log.c
endif

if ENABLE_LOGIND
libshadow_la_SOURCES += logind.c
else
libshadow_la_SOURCES += utmp.c
endif

if !WITH_LIBBSD
libshadow_la_SOURCES += \
	freezero.h \
	freezero.c \
	readpassphrase.h \
	readpassphrase.c
endif

# These files are unneeded for some reason, listed in
# order of appearance:
#
# sources for dbm support (not yet used)

EXTRA_DIST = \
	.indent.pro \
	xgetXXbyYY.c
