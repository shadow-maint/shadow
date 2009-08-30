LANG=$(notdir $(CURDIR))

%.xml: ../%.xml ../po/$(LANG).po
if ENABLE_REGENERATE_MAN
	[ ! -f ../config.xml ] || mv ../config.xml ../config.xml.bak
	xml2po --expand-all-entities -l $(LANG) -p ../po/$(LANG).po -o $@ ../$@
	sed -i 's:\(^<refentry .*\)>:\1 lang="$(LANG)">:' $@
	[ ! -f ../config.xml.bak ] || mv ../config.xml.bak ../config.xml
	sed -i 's/config SYSTEM "config.xml">/config SYSTEM "config.xml">\%config;/' $@
else
	@echo you need to run configure with --enable-man to generate man pages
	@false
endif

config.xml: ../config.xml.in
	$(MAKE) -C .. config.xml
	cp ../config.xml $@

include ../generate_mans.mak

CLEANFILES = .xml2po.mo $(EXTRA_DIST) $(addsuffix .xml,$(EXTRA_DIST))
