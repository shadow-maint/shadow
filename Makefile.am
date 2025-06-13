## Process this file with automake to produce Makefile.in

SUBDIRS = lib

if ENABLE_SUBIDS
SUBDIRS += libsubid
endif

SUBDIRS += src po doc etc tests/unit

if ENABLE_REGENERATE_MAN
SUBDIRS += man
endif

CLEANFILES = man/8.out man/po/remove-potcdate.* man/*/login.defs.d man/*/*.mo

EXTRA_DIST = NEWS README tests/

dist-hook:
	chmod -R u+w $(distdir)/tests
	chmod u+w $(distdir)
	mv $(distdir)/tests/unit $(distdir)/realunittest
	mv $(distdir)/tests/tests $(distdir)/realtests
	rm -rf $(distdir)/tests
	mv $(distdir)/realtests $(distdir)/tests
	rm -rf $(distdir)/tests/unit $(distdir)/tests/Makefile*
	mv $(distdir)/realunittest $(distdir)/tests/unit
