
AUTOMAKE_OPTIONS = 1.0 foreign

noinst_HEADERS = commonio.h defines.h dialchk.h dialup.h \
 faillog.h getdef.h groupio.h md5.h pam_defs.h port.h prototypes.h \
 pwauth.h pwio.h rcsid.h sgroupio.h shadowio.h snprintf.h \
 tcfsio.h

localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" -I. -I$(srcdir) -I.. @DEFS@

# These files are unneeded for some reason, listed in
# order of appearance:
#
# sources which are not really needed (are they in libc???)
# sources for dbm support (not yet used)
# sources for LIBOBJS (which are normally in libc)
# misc header sources

EXTRA_DIST = grdbm.c gsdbm.c pwdbm.c spdbm.c \
 grpack.c gspack.c pwpack.c sppack.c \
 gshadow_.h shadow_.h lastlog_.h snprintf.h

EXTRA_libshadow_a_SOURCESS = grent.c pwent.c \
 mkdir.c rename.c rmdir.c strdup.c strcasecmp.c strerror.c strstr.c \
 putgrent.c putpwent.c putspent.c \
 sgetgrent.c sgetpwent.c sgetspent.c snprintf.c \
 md5.c md5crypt.c

# We build libshadow for our tools.

noinst_LIBRARIES = libshadow.a

libshadow_a_SOURCES = commonio.c dialchk.c dialup.c encrypt.c \
 fputsx.c getdef.c getpass.c groupio.c gshadow.c lockpw.c port.c \
 pwauth.c pwio.c rad64.c sgroupio.c shadow.c shadowio.c utent.c \
 tcfsio.c

libshadow_a_LIBADD = @LIBOBJS@

INCLUDES = -I$(top_srcdir)/lib

# shared library support
libdir = ${exec_prefix}/lib
#lib_PROGRAMS = libshadow.la
lib_LTLIBRARIES = libshadow.la
libshadow_la_SOURCES = ${libshadow_a_SOURCES}
libshadow_la_LIBADD = @LTLIBOBJS@
#libshadow_la_LDFLAGS = -version-info 0:0:0 -rpath $(libdir)
libshadow_la_LDFLAGS = -version-info 0:0:0

# remove the libshadow.so -> libshadow.so.x.x symlink, because this
# library is for internal use by this package only.  Shadow support
# is in libc and no one should be using -lshadow anymore.
install-exec-hook:
	rm -f $(libdir)/libshadow.so

