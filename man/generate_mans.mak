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

if ENABLE_REGENERATE_MAN
%.xml-config: %.xml Makefile
	if grep -q SHADOW-CONFIG-HERE $<; then \
		sed -e 's/^<!-- SHADOW-CONFIG-HERE -->/<!ENTITY % config SYSTEM "config.xml">%config;/' $< > $@; \
	else \
		sed -e 's/^\(<!DOCTYPE .*docbookx.dtd"\)>/\1 [<!ENTITY % config SYSTEM "config.xml">%config;]>/' $< > $@; \
	fi

%: %.xml-config Makefile config.xml
	$(XSLTPROC) --stringparam profile.condition "$(PAM_COND);$(SHADOWGRP_COND);$(TCB_COND);$(SHA_CRYPT_COND)" \
	            -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/profile-docbook.xsl $<
else
$(man_MANS):
	@echo you need to run configure with --enable-man to generate man pages
	@false
endif

grpconv.8 grpunconv.8 pwunconv.8: pwconv.8

getspnam.3: shadow.3

vigr.8: vipw.8
