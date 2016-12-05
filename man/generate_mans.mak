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
if WITH_TCB
TCB_COND=tcb
else
TCB_COND=no_tcb
endif

if USE_SHA_CRYPT
SHA_CRYPT_COND=sha_crypt
else
SHA_CRYPT_COND=no_sha_crypt
endif

if ENABLE_SUBIDS
SUBIDS_COND=subids
else
SUBIDS_COND=no_subids
endif

if ENABLE_REGENERATE_MAN
%.xml-config: %.xml
	if grep -q SHADOW-CONFIG-HERE $<; then \
		sed -e 's/^<!-- SHADOW-CONFIG-HERE -->/<!ENTITY % config SYSTEM "config.xml">%config;/' $< > $@; \
	else \
		sed -e 's/^\(<!DOCTYPE .*docbookx.dtd"\)>/\1 [<!ENTITY % config SYSTEM "config.xml">%config;]>/' $< > $@; \
	fi

man1/% man3/% man5/% man8/%: %.xml-config Makefile config.xml
	$(XSLTPROC) --stringparam profile.condition "$(PAM_COND);$(SHADOWGRP_COND);$(TCB_COND);$(SHA_CRYPT_COND);$(SUBIDS_COND)" \
	            --param "man.authors.section.enabled" "0" \
	            --stringparam "man.output.base.dir" "" \
	            --param "man.output.in.separate.dir" "1" \
	            -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/profile-docbook.xsl $<

clean-local:
	for d in man1 man3 man5 man8; do [ -d $$d ] && rmdir $$d; done

else
$(man_MANS):
	@echo you need to run configure with --enable-man to generate man pages
endif

man8/grpconv.8 man8/grpunconv.8 man8/pwunconv.8: man8/pwconv.8

man3/getspnam.3: man3/shadow.3

man8/vigr.8: man8/vipw.8
