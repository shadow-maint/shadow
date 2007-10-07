
AUTOMAKE_OPTIONS = 1.0 foreign

DEFS = 

lib_LTLIBRARIES = libshadow.la

libshadow_la_LDFLAGS = -version-info 0:0:0
libshadow_la_LIBADD = $(INTLLIBS) $(LIBCRYPT) $(LIBSKEY) $(LIBMD) $(LIBSELINUX)

libshadow_la_SOURCES = \
	commonio.c \
	commonio.h \
	defines.h \
	encrypt.c \
	faillog.h \
	fputsx.c \
	getdef.c \
	getdef.h \
	getpass.c \
	groupio.c \
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
	rcsid.h \
	sgetgrent.c \
	sgetpwent.c \
	sgroupio.c \
	sgroupio.h\
	shadow.c \
	shadowio.c \
	shadowio.h \
	utent.c

# These files are unneeded for some reason, listed in
# order of appearance:
#
# sources for dbm support (not yet used)

EXTRA_DIST = \
	.indent.pro \
	grpack.c gspack.c pwpack.c sppack.c \
	gshadow_.h
