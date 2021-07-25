lib_LTLIBRARIES = libsubid.la
if ENABLE_SHARED
libsubid_la_LDFLAGS = -Wl,-soname,libsubid.so.@LIBSUBID_ABI@ \
	-shared -version-info @LIBSUBID_ABI_MAJOR@
endif
libsubid_la_SOURCES = api.c

pkginclude_HEADERS = subid.h

MISCLIBS = \
	$(LIBAUDIT) \
	$(LIBSELINUX) \
	$(LIBSEMANAGE) \
	$(LIBCRYPT_NOPAM) \
	$(LIBSKEY) \
	$(LIBMD) \
	$(LIBECONF) \
	$(LIBCRYPT) \
	$(LIBACL) \
	$(LIBATTR) \
	$(LIBTCB) \
	$(LIBPAM)

libsubid_la_LIBADD = \
	$(top_builddir)/lib/libshadow.la \
	$(top_builddir)/libmisc/libmisc.la \
	$(MISCLIBS) -ldl

AM_CPPFLAGS = \
	-I${top_srcdir}/lib \
	-I${top_srcdir}/libmisc \
	-DLOCALEDIR=\"$(datadir)/locale\"
