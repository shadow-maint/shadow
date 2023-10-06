AM_CPPFLAGS = -I$(top_srcdir)/lib -I$(top_srcdir)

if HAVE_CMOCKA
TESTS = $(check_PROGRAMS)

check_PROGRAMS = \
    test_xasprintf

if ENABLE_LOGIND
check_PROGRAMS += \
    test_logind
endif # ENABLE_LOGIND

check_PROGRAMS += \
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

test_xasprintf_SOURCES = \
    ../../lib/sprintf.c \
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
