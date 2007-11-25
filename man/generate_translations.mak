if ENABLE_REGENERATE_MAN

LANG=$(notdir $(CURDIR))

%.xml: ../%.xml ../po/$(LANG).po
	xml2po -l $(LANG) -p ../po/$(LANG).po -o $@ ../$@
	sed -i 's:\(^<refentry .*\)>:\1 lang="$(LANG)">:' $@

include ../generate_mans.mak

CLEANFILES = .xml2po.mo $(EXTRA_DIST) $(addsuffix .xml,$(EXTRA_DIST))

endif
