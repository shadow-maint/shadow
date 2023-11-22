AM_CPPFLAGS = -I$(top_srcdir)/lib -I$(top_srcdir)

if HAVE_CMOCKA
TESTS = $(check_PROGRAMS)

check_PROGRAMS = \
    test_chkname \
    test_snprintf \
    test_strncpy \
    test_strtcpy \
    test_xasprintf

if ENABLE_LOGIND
check_PROGRAMS += \
    test_logind
endif # ENABLE_LOGIND

check_PROGRAMS += \
    $(NULL)

test_chkname_SOURCES = \
    ../../lib/chkname.c \
    test_chkname.c \
    $(NULL)
test_chkname_CFLAGS = \
    $(AM_FLAGS) \
    $(NULL)
test_chkname_LDFLAGS = \
    $(NULL)
test_chkname_LDADD = \
    $(CMOCKA_LIBS) \
    $(NULL)

test_logind_SOURCES = \
    ../../lib/logind.c \
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
    $(CMOCKA_LIBS) \
    $(LIBSYSTEMD) \
    $(NULL)

test_snprintf_SOURCES = \
    ../../lib/string/sprintf.c \
    test_snprintf.c \
    $(NULL)
test_snprintf_CFLAGS = \
    $(AM_FLAGS) \
    $(NULL)
test_snprintf_LDFLAGS = \
    $(NULL)
test_snprintf_LDADD = \
    $(CMOCKA_LIBS) \
    $(NULL)

test_strncpy_SOURCES = \
    test_strncpy.c \
    $(NULL)
test_strncpy_CFLAGS = \
    $(AM_FLAGS) \
    $(NULL)
test_strncpy_LDFLAGS = \
    $(NULL)
test_strncpy_LDADD = \
    $(CMOCKA_LIBS) \
    $(NULL)

test_strtcpy_SOURCES = \
    ../../lib/string/strtcpy.c \
    test_strtcpy.c \
    $(NULL)
test_strtcpy_CFLAGS = \
    $(AM_FLAGS) \
    $(NULL)
test_strtcpy_LDFLAGS = \
    $(NULL)
test_strtcpy_LDADD = \
    $(CMOCKA_LIBS) \
    $(NULL)

test_xasprintf_SOURCES = \
    ../../lib/string/sprintf.c \
    test_xasprintf.c \
    $(NULL)
test_xasprintf_CFLAGS = \
    $(AM_FLAGS) \
    $(NULL)
test_xasprintf_LDFLAGS = \
    -Wl,-wrap,vasprintf \
    -Wl,-wrap,exit \
    $(NULL)
test_xasprintf_LDADD = \
    $(CMOCKA_LIBS) \
    $(NULL)

endif # HAVE_CMOCKA
