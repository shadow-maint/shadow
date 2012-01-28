
mandir = @mandir@/it

# 2012.01.28 - activate manpages with more than 50% translated messages
man_MANS = \
	chage.1 \
	chsh.1 \
	expiry.1 \
	faillog.5 \
	faillog.8 \
	getspnam.3 \
	groupdel.8 \
	groupmod.8 \
	groups.1 \
	grpck.8 \
	lastlog.8 \
	logoutd.8 \
	newgrp.1 \
	nologin.8 \
	pwck.8 \
	sg.1 \
	shadow.3 \
	userdel.8 \
	vigr.8 \
	vipw.8

man_nopam = \
	porttime.5

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

