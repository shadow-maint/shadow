lib_LTLIBRARIES = libsubid.la
libsubid_la_LDFLAGS = -Wl,-soname,libsubid.so.@LIBSUBID_ABI@ \
	-shared -version-info @LIBSUBID_ABI_MAJOR@
libsubid_la_SOURCES = api.c

MISCLIBS = \
	$(LIBAUDIT) \
	$(LIBSELINUX) \
	$(LIBSEMANAGE) \
	$(LIBCRYPT_NOPAM) \
	$(LIBSKEY) \
	$(LIBMD) \
	$(LIBECONF) \
	$(LIBCRYPT) \
	$(LIBTCB)

libsubid_la_LIBADD = \
	$(top_srcdir)/lib/libshadow.la \
	$(top_srcdir)/libmisc/libmisc.a \
	$(MISCLIBS)

AM_CPPFLAGS = \
	-I${top_srcdir}/lib \
	-I${top_srcdir}/libmisc \
	-DLOCALEDIR=\"$(datadir)/locale\"
