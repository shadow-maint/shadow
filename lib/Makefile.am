
AUTOMAKE_OPTIONS = 1.0 foreign

DEFS = 
INCLUDES = -I$(top_srcdir)

lib_LTLIBRARIES = libshadow.la

libshadow_la_LDFLAGS = -version-info 0:0:0
libshadow_la_LIBADD = $(INTLLIBS) $(LIBCRYPT) $(LIBTCFS) $(LIBSKEY) $(LIBMD) \
	$(top_builddir)/libmisc/libmisc.la

libshadow_la_SOURCES = \
	commonio.c \
	commonio.h \
	defines.h \
	dialchk.c \
	dialchk.h \
	dialup.c \
	dialup.h \
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
	md5.h \
	pam_defs.h \
	port.c \
	port.h \
	prototypes.h \
	pwauth.c \
	pwauth.h \
	pwio.c \
	pwio.h \
	rad64.c \
	rcsid.h \
	sgetgrent.c \
	sgetpwent.c \
	sgroupio.c \
	sgroupio.h\
	shadow.c \
	shadowio.c \
	shadowio.h \
	snprintf.h \
	tcfsio.c \
	tcfsio.h \
	utent.c

EXTRA_libshadow_la_SOURCESS = grent.c pwent.c mkdir.c rename.c rmdir.c \
	strdup.c strcasecmp.c strerror.c strstr.c putgrent.c \
	putpwent.c putspent.c sgetgrent.c sgetpwent.c sgetspent.c \
	snprintf.c md5.c md5crypt.c

# These files are unneeded for some reason, listed in
# order of appearance:
#
# sources which are not really needed (are they in libc???)
# sources for dbm support (not yet used)
# sources for LIBOBJS (which are normally in libc)
# misc header sources

EXTRA_DIST = \
	.indent.pro \
	grdbm.c gsdbm.c pwdbm.c spdbm.c \
	grpack.c gspack.c pwpack.c sppack.c \
	gshadow_.h shadow_.h lastlog_.h snprintf.h
