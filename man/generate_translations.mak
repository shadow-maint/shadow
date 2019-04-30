LANG=$(notdir $(CURDIR))

if ENABLE_REGENERATE_MAN
config.xml: ../config.xml.in
	$(MAKE) -C .. config.xml
	cp ../config.xml $@

%.xml: ../%.xml-config ../po/$(LANG).po
	msgfmt -o $(LANG).mo ../po/$(LANG).po
	itstool -l $(LANG) -m $(LANG).mo -o $@ $<
	sed -i 's:\(^<refentry .*\)>:\1 lang="$(LANG)">:' $@

include ../generate_mans.mak

else
$(man_MANS):
	@echo you need to run configure with --enable-man to generate man pages
endif

CLEANFILES = $(EXTRA_DIST) $(addsuffix .xml,$(EXTRA_DIST)) config.xml
