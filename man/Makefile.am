
# subdirectories for translated manual pages
if USE_NLS
SUBDIRS = po cs de es fi fr hu id it ja ko pl pt_BR ru sv tr zh_CN zh_TW
else
SUBDIRS =
endif

man_MANS = \
	chage.1 \
	chfn.1 \
	chgpasswd.8 \
	chpasswd.8 \
	chsh.1 \
	expiry.1 \
	faillog.5 \
	faillog.8 \
	getspnam.3 \
	gpasswd.1 \
	groupadd.8 \
	groupdel.8 \
	groupmems.8 \
	groupmod.8 \
	groups.1 \
	grpck.8 \
	grpconv.8 \
	grpunconv.8 \
	gshadow.5 \
	lastlog.8 \
	login.1 \
	login.defs.5 \
	logoutd.8 \
	newgrp.1 \
	newusers.8 \
	nologin.8 \
	passwd.1 \
	passwd.5 \
	pwck.8 \
	pwconv.8 \
	pwunconv.8 \
	sg.1 \
	shadow.3 \
	shadow.5 \
	su.1 \
	suauth.5 \
	useradd.8 \
	userdel.8 \
	usermod.8 \
	vigr.8 \
	vipw.8

man_nopam = \
	limits.5 \
	login.access.5 \
	porttime.5

if !USE_PAM
man_MANS += $(man_nopam)
endif

man_XMANS = \
	chage.1.xml \
	chfn.1.xml \
	chgpasswd.8.xml \
	chpasswd.8.xml \
	chsh.1.xml \
	expiry.1.xml \
	faillog.5.xml \
	faillog.8.xml \
	gpasswd.1.xml \
	groupadd.8.xml \
	groupdel.8.xml \
	groupmems.8.xml \
	groupmod.8.xml \
	groups.1.xml \
	grpck.8.xml \
	gshadow.5.xml \
	lastlog.8.xml \
	limits.5.xml \
	login.1.xml \
	login.access.5.xml \
	login.defs.5.xml \
	logoutd.8.xml \
	newgrp.1.xml \
	newusers.8.xml \
	nologin.8.xml \
	passwd.1.xml \
	passwd.5.xml \
	porttime.5.xml \
	pwck.8.xml \
	pwconv.8.xml \
	shadow.3.xml \
	shadow.5.xml \
	sg.1.xml \
	su.1.xml \
	suauth.5.xml \
	useradd.8.xml \
	userdel.8.xml \
	usermod.8.xml \
	vipw.8.xml

EXTRA_DIST = \
	$(man_MANS) \
	$(man_XMANS) \
	$(man_nopam) \
	id.1 \
	id.1.xml \
	sulogin.8 \
	sulogin.8.xml \
	generate_mans.mak \
	generate_translations.mak

if ENABLE_REGENERATE_MAN

include generate_mans.mak

CLEANFILES = $(man_MANS)

else
$(man_MANS):
	@echo ""
	@echo "Error: you need to run configure with '--enable-man'"
	@echo "       in order to regenerate man pages."
	@echo ""
	@false
endif
