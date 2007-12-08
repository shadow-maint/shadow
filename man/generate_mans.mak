if ENABLE_REGENERATE_MAN

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

if USE_SHA_CRYPT
SHA_CRYPT_COND=sha_crypt
else
SHA_CRYPT_COND=no_sha_crypt
endif

%: %.xml Makefile
	$(XSLTPROC) --stringparam profile.condition "$(PAM_COND);$(SHADOWGRP_COND);$(SHA_CRYPT_COND)" \
	            -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/profile-docbook.xsl $<

grpconv.8 grpunconv.8 pwunconv.8: pwconv.8

getspnam.3: shadow.3

vigr.8: vipw.8

endif
