LANG=$(notdir $(CURDIR))

if ENABLE_REGENERATE_MAN
config.xml: ../config.xml.in
	$(MAKE) -C .. config.xml
	cp ../config.xml $@

%.xml: ../%.xml ../po/$(LANG).po
	xml2po --expand-all-entities -l $(LANG) -p ../po/$(LANG).po -o $@ ../$@
	sed -i 's:\(^<refentry .*\)>:\1 lang="$(LANG)">:' $@

include ../generate_mans.mak

else
$(man_MANS):
	@echo you need to run configure with --enable-man to generate man pages
	@false
endif

CLEANFILES = .xml2po.mo $(EXTRA_DIST) $(addsuffix .xml,$(EXTRA_DIST)) config.xml
