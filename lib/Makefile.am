
AUTOMAKE_OPTIONS = 1.0 foreign

DEFS = 

noinst_LTLIBRARIES = libshadow.la

libshadow_la_LDFLAGS = -version-info 0:0:0
libshadow_la_LIBADD = $(INTLLIBS) $(LIBCRYPT) $(LIBSKEY) $(LIBMD)

libshadow_la_SOURCES = \
	commonio.c \
	commonio.h \
	defines.h \
	encrypt.c \
	exitcodes.h \
	faillog.h \
	fputsx.c \
	getdef.c \
	getdef.h \
	groupio.c \
	groupmem.c \
	groupio.h \
	gshadow.c \
	lockpw.c \
	nscd.c \
	nscd.h \
	pam_defs.h \
	port.c \
	port.h \
	prototypes.h \
	pwauth.c \
	pwauth.h \
	pwio.c \
	pwio.h \
	pwmem.c \
	sgetgrent.c \
	sgetpwent.c \
	sgroupio.c \
	sgroupio.h\
	shadow.c \
	shadowio.c \
	shadowio.h \
	shadowmem.c \
	utent.c

# These files are unneeded for some reason, listed in
# order of appearance:
#
# sources for dbm support (not yet used)

EXTRA_DIST = \
	.indent.pro \
	gshadow_.h
