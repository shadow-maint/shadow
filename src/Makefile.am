
EXTRA_DIST = \
	.indent.pro

ubindir = ${prefix}/bin
usbindir = ${prefix}/sbin

INCLUDES = \
	-I${top_srcdir}/lib \
	-I$(top_srcdir)/libmisc

# XXX why are login and su in /bin anyway (other than for
# historical reasons)?
#
# if the system is screwed so badly that it can't mount /usr,
# you can (hopefully) boot single user, and then you're root
# so you don't need these programs for recovery.
#
# also /lib/libshadow.so.x.xx (if any) could be moved to /usr/lib
# and installation would be much simpler (just two directories,
# $prefix/bin and $prefix/sbin, no install-data hacks...)

bin_PROGRAMS   = groups login su
ubin_PROGRAMS  = faillog lastlog chage chfn chsh expiry gpasswd newgrp passwd
usbin_PROGRAMS = \
	chgpasswd \
	chpasswd \
	groupadd \
	groupdel \
	groupmod \
	grpck \
	grpconv \
	grpunconv \
	logoutd \
	newusers \
	nologin \
	pwck \
	pwconv \
	pwunconv \
	useradd \
	userdel \
	usermod \
	vipw

# id and groups are from gnu, sulogin from sysvinit
noinst_PROGRAMS = id sulogin

suidbins       = su
suidubins      = chage chfn chsh expiry gpasswd newgrp passwd

LDADD 	       = $(top_builddir)/libmisc/libmisc.a \
		 $(top_builddir)/lib/libshadow.la
AM_CPPFLAGS    = -DLOCALEDIR=\"$(datadir)/locale\"

chage_LDADD    = $(LDADD) $(LIBPAM) $(LIBAUDIT) $(LIBSELINUX)
chfn_LDADD     = $(LDADD) $(LIBPAM) $(LIBSELINUX)
chgpasswd_LDADD = $(LDADD) $(LIBPAM) $(LIBSELINUX)
chsh_LDADD     = $(LDADD) $(LIBPAM) $(LIBSELINUX)
chpasswd_LDADD = $(LDADD) $(LIBPAM) $(LIBSELINUX)
gpasswd_LDADD  = $(LDADD) $(LIBAUDIT) $(LIBSELINUX)
groupadd_LDADD = $(LDADD) $(LIBPAM) $(LIBAUDIT) $(LIBSELINUX)
groupdel_LDADD = $(LDADD) $(LIBPAM) $(LIBAUDIT) $(LIBSELINUX)
groupmod_LDADD = $(LDADD) $(LIBPAM) $(LIBAUDIT) $(LIBSELINUX)
grpck_LDADD    = $(LDADD) $(LIBSELINUX)
grpconv_LDADD  = $(LDADD) $(LIBSELINUX)
grpunconv_LDADD = $(LDADD) $(LIBSELINUX)
login_SOURCES	= \
	login.c \
	login_nopam.c
login_LDADD    = $(LDADD) $(LIBPAM) $(LIBAUDIT)
newgrp_LDADD   = $(LDADD) $(LIBPAM) $(LIBAUDIT)
newusers_LDADD = $(LDADD) $(LIBPAM) $(LIBSELINUX)
nologin_LDADD  =
passwd_LDADD   = $(LDADD) $(LIBPAM) $(LIBCRACK) $(LIBAUDIT) $(LIBSELINUX)
pwck_LDADD     = $(LDADD) $(LIBSELINUX)
pwconv_LDADD   = $(LDADD) $(LIBSELINUX)
pwunconv_LDADD = $(LDADD) $(LIBSELINUX)
su_SOURCES     = \
	su.c \
	suauth.c
su_LDADD       = $(LDADD) $(LIBPAM)
useradd_LDADD  = $(LDADD) $(LIBPAM) $(LIBAUDIT) $(LIBSELINUX)
userdel_LDADD  = $(LDADD) $(LIBPAM) $(LIBAUDIT) $(LIBSELINUX)
usermod_LDADD  = $(LDADD) $(LIBPAM) $(LIBAUDIT) $(LIBSELINUX)
vipw_LDADD     = $(LDADD) $(LIBSELINUX)

install-am: all-am
	$(MAKE) $(AM_MAKEFLAGS) install-exec-am install-data-am
	ln -sf newgrp	$(DESTDIR)$(ubindir)/sg
	ln -sf vipw	$(DESTDIR)$(usbindir)/vigr
	for i in $(suidbins); do \
		chmod -f 4755 $(DESTDIR)$(bindir)/$$i; \
	done
	for i in $(suidubins); do \
		chmod -f 4755 $(DESTDIR)$(ubindir)/$$i; \
	done
