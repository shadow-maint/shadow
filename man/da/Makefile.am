
mandir = @mandir@/da

# 2012.01.28 - activate manpages with more than 50% translated messages
man_MANS = \
	chfn.1 \
	groupdel.8 \
	groups.1 \
	gshadow.5 \
	logoutd.8 \
	newgrp.1 \
	nologin.8 \
	sg.1 \
	vigr.8 \
	vipw.8

man_nopam = 

if !USE_PAM
man_MANS += $(man_nopam)
endif

EXTRA_DIST = \
	$(man_MANS) \
	id.1

if USE_PAM
EXTRA_DIST += $(man_nopam)
endif

include ../generate_translations.mak

