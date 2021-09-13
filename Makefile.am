## Process this file with automake to produce Makefile.in

EXTRA_DIST = NEWS README TODO shadow.spec.in

SUBDIRS = libmisc lib

if ENABLE_SUBIDS
SUBDIRS += libsubid
endif

SUBDIRS += src po contrib doc etc

if ENABLE_REGENERATE_MAN
SUBDIRS += man
endif
