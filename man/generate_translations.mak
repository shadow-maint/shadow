if ENABLE_REGENERATE_MAN

LANG=$(notdir $(CURDIR))

%.xml: ../%.xml $(LANG).po
	xml2po -l $(LANG) -p $(LANG).po -o $@ ../$@
	sed -i 's:\(^<refentry .*\)>:\1 lang="$(LANG)">:' $@

if USE_PAM
PAM_COND=pam
else
PAM_COND=no_pam
endif
if SHADOWGRP
SHADOWGRP_COND=gshadow
else
SHADOWGRP_COND=no_gshadow
endif

%: %.xml
	$(XSLTPROC) --stringparam profile.condition "$(PAM_COND);$(SHADOWGRP_COND)" \
	            -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/profile-docbook.xsl $<

grpconv.8 grpunconv.8 pwunconv.8: pwconv.8

getspnam.3: shadow.3

vigr.8: vipw.8

CLEANFILES = .xml2po.mo $(EXTRA_DIST) $(addsuffix .xml,$(EXTRA_DIST))

endif
