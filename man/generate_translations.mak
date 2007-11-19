if ENABLE_REGENERATE_MAN

LANG=$(notdir $(CURDIR))

%.xml: ../%.xml $(LANG).po
	xml2po -l $(LANG) -p $(LANG).po -o $@ ../$@
	sed -i 's:\(^<refentry .*\)>:\1 lang="$(LANG)">:' $@

%: %.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

grpconv.8 grpunconv.8 pwunconv.8: pwconv.8

getspnam.3: shadow.3

vigr.8: vipw.8

CLEANFILES = .xml2po.mo $(EXTRA_DIST) $(addsuffix .xml,$(EXTRA_DIST))

endif
