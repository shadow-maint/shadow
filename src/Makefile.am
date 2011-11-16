
EXTRA_DIST = \
	.indent.pro

ubindir = ${prefix}/bin
usbindir = ${prefix}/sbin
suidperms = 4755
sgidperms = 2755

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
sbin_PROGRAMS  = nologin
ubin_PROGRAMS  = faillog lastlog chage chfn chsh expiry gpasswd newgrp passwd
usbin_PROGRAMS = \
	chgpasswd \
	chpasswd \
	groupadd \
	groupdel \
	groupmems \
	groupmod \
	grpck \
	grpconv \
	grpunconv \
	logoutd \
	newusers \
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
if ACCT_TOOLS_SETUID
	suidubins += chage chgpasswd chpasswd groupadd groupdel groupmod newusers useradd userdel usermod
endif

if WITH_TCB
suidubins -= passwd
shadowsgidubins = passwd
endif

LDADD          = $(INTLLIBS) \
		 $(LIBTCB) \
		 $(top_builddir)/libmisc/libmisc.a \
		 $(top_builddir)/lib/libshadow.la
AM_CPPFLAGS    = -DLOCALEDIR=\"$(datadir)/locale\"

if ACCT_TOOLS_SETUID
LIBPAM_SUID  = $(LIBPAM)
else
LIBPAM_SUID  =
endif

if USE_PAM
LIBCRYPT_NOPAM =
else
LIBCRYPT_NOPAM = $(LIBCRYPT)
endif

chage_LDADD    = $(LDADD) $(LIBPAM_SUID) $(LIBAUDIT) $(LIBSELINUX)
chfn_LDADD     = $(LDADD) $(LIBPAM) $(LIBSELINUX) $(LIBCRYPT_NOPAM) $(LIBSKEY) $(LIBMD)
chgpasswd_LDADD = $(LDADD) $(LIBPAM_SUID) $(LIBSELINUX) $(LIBCRYPT)
chsh_LDADD     = $(LDADD) $(LIBPAM) $(LIBSELINUX) $(LIBCRYPT_NOPAM) $(LIBSKEY) $(LIBMD)
chpasswd_LDADD = $(LDADD) $(LIBPAM) $(LIBSELINUX) $(LIBCRYPT)
gpasswd_LDADD  = $(LDADD) $(LIBAUDIT) $(LIBSELINUX) $(LIBCRYPT)
groupadd_LDADD = $(LDADD) $(LIBPAM_SUID) $(LIBAUDIT) $(LIBSELINUX)
groupdel_LDADD = $(LDADD) $(LIBPAM_SUID) $(LIBAUDIT) $(LIBSELINUX)
groupmems_LDADD = $(LDADD) $(LIBPAM) $(LIBSELINUX)
groupmod_LDADD = $(LDADD) $(LIBPAM_SUID) $(LIBAUDIT) $(LIBSELINUX)
grpck_LDADD    = $(LDADD) $(LIBSELINUX)
grpconv_LDADD  = $(LDADD) $(LIBSELINUX)
grpunconv_LDADD = $(LDADD) $(LIBSELINUX)
login_SOURCES  = \
	login.c \
	login_nopam.c
login_LDADD    = $(LDADD) $(LIBPAM) $(LIBAUDIT) $(LIBCRYPT_NOPAM) $(LIBSKEY) $(LIBMD)
newgrp_LDADD   = $(LDADD) $(LIBAUDIT) $(LIBCRYPT)
newusers_LDADD = $(LDADD) $(LIBPAM) $(LIBSELINUX) $(LIBCRYPT)
nologin_LDADD  =
passwd_LDADD   = $(LDADD) $(LIBPAM) $(LIBCRACK) $(LIBAUDIT) $(LIBSELINUX) $(LIBCRYPT_NOPAM)
pwck_LDADD     = $(LDADD) $(LIBSELINUX)
pwconv_LDADD   = $(LDADD) $(LIBSELINUX)
pwunconv_LDADD = $(LDADD) $(LIBSELINUX)
su_SOURCES     = \
	su.c \
	suauth.c
su_LDADD       = $(LDADD) $(LIBPAM) $(LIBCRYPT_NOPAM) $(LIBSKEY) $(LIBMD)
sulogin_LDADD  = $(LDADD) $(LIBCRYPT)
useradd_LDADD  = $(LDADD) $(LIBPAM_SUID) $(LIBAUDIT) $(LIBSELINUX) $(LIBACL) $(LIBATTR)
userdel_LDADD  = $(LDADD) $(LIBPAM_SUID) $(LIBAUDIT) $(LIBSELINUX)
usermod_LDADD  = $(LDADD) $(LIBPAM_SUID) $(LIBAUDIT) $(LIBSELINUX) $(LIBACL) $(LIBATTR)
vipw_LDADD     = $(LDADD) $(LIBSELINUX)

install-am: all-am
	$(MAKE) $(AM_MAKEFLAGS) install-exec-am install-data-am
	ln -sf newgrp	$(DESTDIR)$(ubindir)/sg
	ln -sf vipw	$(DESTDIR)$(usbindir)/vigr
	for i in $(suidbins); do \
		chmod -f $(suidperms) $(DESTDIR)$(bindir)/$$i; \
	done
	for i in $(suidubins); do \
		chmod -f $(suidperms) $(DESTDIR)$(ubindir)/$$i; \
	done
if WITH_TCB
	for i in $(shadowsgidubins); do \
		chown root:shadow $(DESTDIR)$(ubindir)/$$i; \
		chmod -f $(sgidperms) $(DESTDIR)$(ubindir)/$$i; \
	done
endif
