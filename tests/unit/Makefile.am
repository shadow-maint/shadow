AM_CPPFLAGS = -I$(top_srcdir)/lib -I$(top_srcdir)

LIBSHADOW = $(top_builddir)/lib/libshadow.la

if HAVE_CMOCKA
TESTS = $(check_PROGRAMS)

check_PROGRAMS = \
    test_adds \
    test_atoi_strtoi \
    test_chkhash \
    test_chkname \
    test_stprintf \
    test_strtcpy \
    test_typetraits \
    test_exit_if_null

if ENABLE_LOGIND
check_PROGRAMS += \
    test_logind
endif # ENABLE_LOGIND

check_PROGRAMS += \
    $(NULL)

test_adds_SOURCES = \
    test_adds.c \
    $(NULL)
test_adds_CFLAGS = \
    $(AM_FLAGS) \
    $(NULL)
test_adds_LDFLAGS = \
    $(NULL)
test_adds_LDADD = \
    $(LIBSHADOW) \
    $(CMOCKA_LIBS) \
    $(NULL)

test_atoi_strtoi_SOURCES = \
    test_atoi_strtoi.c \
    $(NULL)
test_atoi_strtoi_CFLAGS = \
    $(AM_CFLAGS) \
    $(NULL)
test_atoi_strtoi_LDFLAGS = \
    $(NULL)
test_atoi_strtoi_LDADD = \
    $(LIBSHADOW) \
    $(CMOCKA_LIBS) \
    $(NULL)

test_chkhash_SOURCES = \
    test_chkhash.c \
    $(NULL)
test_chkhash_CFLAGS = \
    $(AM_CFLAGS) \
    $(NULL)
test_chkhash_LDFLAGS = \
    $(NULL)
test_chkhash_LDADD = \
    $(LIBSHADOW) \
    $(CMOCKA_LIBS) \
    $(NULL)

test_chkname_SOURCES = \
    test_chkname.c \
    $(NULL)
test_chkname_CFLAGS = \
    $(AM_CFLAGS) \
    $(NULL)
test_chkname_LDFLAGS = \
    $(NULL)
test_chkname_LDADD = \
    $(LIBSHADOW) \
    $(CMOCKA_LIBS) \
    $(NULL)

test_logind_SOURCES = \
    test_logind.c \
    $(NULL)
test_logind_CFLAGS = \
    $(AM_CFLAGS) \
    -lsystemd \
    $(NULL)
test_logind_LDFLAGS = \
    -Wl,-wrap,prefix_getpwnam \
    -Wl,-wrap,sd_uid_get_sessions \
    $(NULL)
test_logind_LDADD = \
    $(LIBSHADOW) \
    $(CMOCKA_LIBS) \
    $(LIBSYSTEMD) \
    $(NULL)

test_stprintf_SOURCES = \
    test_stprintf.c \
    $(NULL)
test_stprintf_CFLAGS = \
    $(AM_CFLAGS) \
    -Wno-format-zero-length \
    $(NULL)
test_stprintf_LDFLAGS = \
    $(NULL)
test_stprintf_LDADD = \
    $(LIBSHADOW) \
    $(CMOCKA_LIBS) \
    $(NULL)

test_strtcpy_SOURCES = \
    test_strtcpy.c \
    $(NULL)
test_strtcpy_CFLAGS = \
    $(AM_CFLAGS) \
    $(NULL)
test_strtcpy_LDFLAGS = \
    $(NULL)
test_strtcpy_LDADD = \
    $(LIBSHADOW) \
    $(CMOCKA_LIBS) \
    $(NULL)

test_typetraits_SOURCES = \
    test_typetraits.c \
    $(NULL)
test_typetraits_CFLAGS = \
    $(AM_CFLAGS) \
    $(NULL)
test_typetraits_LDFLAGS = \
    $(NULL)
test_typetraits_LDADD = \
    $(CMOCKA_LIBS) \
    $(NULL)

test_exit_if_null_SOURCES = \
    test_exit_if_null.c \
    $(NULL)
test_exit_if_null_CFLAGS = \
    $(AM_CFLAGS) \
    $(NULL)
test_exit_if_null_LDFLAGS = \
    -Wl,-wrap,exit \
    $(NULL)
test_exit_if_null_LDADD = \
    $(LIBSHADOW) \
    $(CMOCKA_LIBS) \
    $(NULL)

endif # HAVE_CMOCKA
