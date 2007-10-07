
EXTRA_DOST = .indent.pro

DEFS = -DHAVE_CONFIG_H

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/lib

lib_LTLIBRARIES = libmisc.la

libmisc_la_SOURCES = \
	addgrps.c \
	age.c \
	basename.c \
	chkname.c \
	chkname.h \
	chkshell.c \
	chowndir.c \
	chowntty.c \
	console.c \
	copydir.c \
	entry.c \
	env.c \
	failure.c \
	failure.h \
	fields.c \
	getdate.y \
	getdate.h \
	hushed.c \
	isexpired.c \
	limits.c \
	list.c log.c \
	login_access.c \
	login_desrpc.c \
	login_krb.c \
	loginprompt.c \
	mail.c \
	motd.c \
	myname.c \
	nscd.c \
	obscure.c \
	pam_pass.c \
	pwd2spwd.c \
	pwdcheck.c \
	pwd_init.c \
	rlogin.c \
	salt.c \
	setugid.c \
	setup.c \
	setupenv.c \
	shell.c \
	strtoday.c \
	sub.c \
	sulog.c \
	ttytype.c \
	tz.c \
	ulimit.c \
	utmp.c \
	valid.c \
	xmalloc.c

libmisc_la_LIBADD = $(LIBCRACK)
