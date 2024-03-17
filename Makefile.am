## Process this file with automake to produce Makefile.in

EXTRA_DIST = NEWS README

SUBDIRS = lib

if ENABLE_SUBIDS
SUBDIRS += libsubid
endif

SUBDIRS += src po contrib doc etc tests/unit

if ENABLE_REGENERATE_MAN
SUBDIRS += man

install-man:
	$(MAKE) -C man install-man
endif
