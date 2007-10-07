
AUTOMAKE_OPTIONS = 1.0 foreign

noinst_HEADERS = chkname.h failure.h getdate.h

noinst_LIBRARIES = libmisc.a

libdir = $(prefix)/lib
localedir = $(datadir)/locale
INCLUDES = -I$(top_srcdir)/libmisc -I$(top_srcdir)/lib
DEFS = -DLOCALEDIR=\"$(localedir)\" -I. -I$(srcdir) -I.. @DEFS@

libmisc_a_SOURCES = \
	addgrps.c \
	age.c \
	basename.c \
	chkname.c \
	chkshell.c \
	chowndir.c \
	chowntty.c \
	console.c \
	copydir.c \
	entry.c \
	env.c \
	failure.c \
	fields.c \
	getdate.y \
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
	obscure.c \
	pam_pass.c \
	pwd2spwd.c \
	pwd_init.c \
	rlogin.c \
	salt.c \
	setugid.c \
	setup.c \
	setupenv.c \
	shell.c \
	strtoday.c \
	suauth.c \
	sub.c \
	sulog.c \
	ttytype.c \
	tz.c \
	ulimit.c \
	utmp.c \
	valid.c \
	xmalloc.c

