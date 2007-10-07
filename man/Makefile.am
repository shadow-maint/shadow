
LINGUAS = fr pl ru

# subdirectories for translated manual pages
DIST_SUBDIRS = cs de es fi fr hu id it ja ko pl pt_BR ru tr zh_CN zh_TW
if USE_NLS
SUBDIRS = $(DIST_SUBDIRS)
else
SUBDIRS =
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

if USE_PAM
man_nonpam =
else
man_nonpam = \
	limits.5 \
	login.access.5 \
	porttime.5
endif

man_MANS = \
	$(man_nonpam) \
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

EXTRA_DIST = \
	$(man_MANS) \
	$(man_XMANS) \
	$(man_nonpam) \
	id.1 \
	id.1.xml \
	sulogin.8 \
	sulogin.8.xml

if ENABLE_REGENERATE_MAN

chage.1: chage.1.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

chfn.1: chfn.1.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

chgpasswd.8: chgpasswd.8.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

chpasswd.8: chpasswd.8.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

chsh.1: chsh.1.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

expiry.1: expiry.1.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

faillog.5: faillog.5.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

faillog.8: faillog.8.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

gpasswd.1: gpasswd.1.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

groupadd.8: groupadd.8.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

groupdel.8: groupdel.8.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

groupmems.8: groupmems.8.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

groupmod.8: groupmod.8.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

groups.1: groups.1.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

grpck.8: grpck.8.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

gshadow.5: gshadow.5.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

id.1: id.1.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

lastlog.8: lastlog.8.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

limits.5: limits.5.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

login.1: login.1.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

login.access.5: login.access.5.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

login.defs.5: login.defs.5.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

logoutd.8: logoutd.8.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

newgrp.1: newgrp.1.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

newusers.8: newusers.8.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

nologin.8: nologin.8.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

passwd.1: passwd.1.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

passwd.5: passwd.5.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

porttime.5: porttime.5.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

pwck.8: pwck.8.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

grpconv.8 grpunconv.8 pwconv.8 pwunconv.8: pwconv.8.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

shadow.3 getspnam.3: shadow.3.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

shadow.5: shadow.5.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

sg.1: sg.1.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

su.1: su.1.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

sulogin.8: sulogin.8.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

suauth.5: suauth.5.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

useradd.8: useradd.8.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

userdel.8: userdel.8.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

usermod.8: usermod.8.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

vigr.8 vipw.8: vipw.8.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

CLEANFILES = $(man_MANS) $(ALL_TRANSLATED_XMLS)

POFILES = $(foreach lang, $(LINGUAS), $(lang)/$(lang).po)
ALL_TRANSLATED_XMLS = $(foreach dir, $(LINGUAS), $(foreach xmlfile, $(man_XMANS), $(dir)/$(xmlfile)))

all: $(POFILES) $(ALL_TRANSLATED_XMLS)

gen-xmls: $(ALL_TRANSLATED_XMLS)

$(ALL_TRANSLATED_XMLS): $(man_XMANS)
	xml2po -l $(strip $(subst /,, $(dir $@))) -p $(strip $(subst /,, $(dir $@)))/$(strip $(subst /,, $(dir $@))).po -o $@ $(notdir $@)
	sed -i 's:\(^<refentry .*\)>:\1 lang="$(strip $(subst /,, $(dir $@)))">:' $@

$(POFILES): shadow-man-pages.pot

shadow-man-pages.pot: $(man_XMANS)

dist-hook: dist-po-files

.PHONY: dist-po-files
dist-po-files: $(POFILES) shadow-man-pages.pot
	@for lang in $(LINGUAS); do \
	  echo " $(mkinstalldirs) $(distdir)/$$lang"; \
	  $(mkinstalldirs) "$(distdir)/$$lang"; \
	done
	@for file in $(POFILES); do \
	  echo "$(INSTALL_DATA) $(srcdir)/$$file $(distdir)/$$file"; \
	  $(INSTALL_DATA) "$(srcdir)/$$file" "$(distdir)/$$file"; \
	done
	$(INSTALL_DATA) "$(srcdir)/shadow-man-pages.pot" "$(distdir)/shadow-man-pages.pot"

shadow-man-pages.pot: $(man_XMANS)
	xml2po -o $@ $(man_XMANS)

.PHONY: update-po
update-po: shadow-man-pages.pot
	@for lang in $(LINGUAS); do \
	  echo "$$lang:"; \
	  echo "$(MSGMERGE) -o $$lang/$$lang.po $$lang/$$lang.po shadow-man-pages.pot"; \
	  $(MSGMERGE) -o $$lang/$$lang.po $$lang/$$lang.po shadow-man-pages.pot; \
	done

else
$(man_MANS):
	@echo ""
	@echo "Error: you need to run configure with '--enable-man'"
	@echo "       in order to regenerate man pages."
	@echo ""
	@false
endif
