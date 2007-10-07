
AUTOMAKE_OPTIONS = 1.0 foreign

# Watch out; note the difference between prefix & exec_prefix.
# Normally configure sets exec_prefix to root when prefix is /usr.

bindir = ${exec_prefix}/bin
sbindir = ${exec_prefix}/sbin
ubindir = ${prefix}/bin
usbindir = ${prefix}/sbin
localedir = $(datadir)/locale

noinst_HEADERS = patchlevel.h

DEFS = -DLOCALEDIR=\"$(localedir)\" -I. -I$(srcdir) -I.. @DEFS@

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

bin_PROGRAMS = login \
 su
ubin_PROGRAMS = faillog lastlog \
 chage chfn chsh expiry gpasswd newgrp passwd
usbin_PROGRAMS = chpasswd dpasswd groupadd groupdel groupmod \
 logoutd mkpasswd newusers \
 useradd userdel usermod grpck pwck vipw \
 grpconv grpunconv pwconv pwunconv

EXTRA_DIST = shadowconfig.sh

# id and groups are from gnu, sulogin from sysvinit,
# also suid programs are installed by hand.
# XXX installation by hand breaks libtool shared lib support
# (the wrapper scripts get installed instead of binaries),
# so we now chmod the programs by hand after normal installation.

suidbins = su
suidubins = chage chfn chsh expiry gpasswd newgrp passwd

install-exec-hook:
	for i in $(suidbins); do \
		chmod 4755 $(bindir)/$$i; \
	done

install-data-hook:
	for i in $(suidubins); do \
		chmod 4755 $(ubindir)/$$i; \
	done
	rm -f $(ubindir)/sg
	ln -s newgrp $(ubindir)/sg

noinst_PROGRAMS = groups id sulogin

#install-exec-local:
#	$(mkinstalldirs) $(bindir)
#	for i in $(suidbins); do \
#	  $(INSTALL) -m 4755 $$i $(bindir); \
#	done
#	$(mkinstalldirs) $(ubindir)
#	for i in $(suidubins); do \
#	  $(INSTALL) -m 4755 $$i $(ubindir); \
#	done
#	rm -f $(bindir)/sg
#	ln -s $(ubindir)/newgrp $(bindir)/sg
#
#noinst_PROGRAMS = id groups \
# su \
# chage chfn chsh expiry gpasswd newgrp passwd \
# sulogin

shlibs = ../lib/libshadow.la
# With glibc2, almost all programs need libcrypt for some reason,
# even those that don't actually use crypt().
LDADD = ${shlibs} ../libmisc/libmisc.a ../lib/libshadow.a @INTLLIBS@ @LIBCRYPT@ @LIBTCFS@ @LIBSKEY@
INCLUDES = -I${top_srcdir}/lib -I$(top_srcdir)/libmisc

chfn_LDADD = ${LDADD} @LIBPAM@
chsh_LDADD = ${LDADD} @LIBPAM@
login_LDADD = ${LDADD} @LIBPAM@
passwd_LDADD = ${LDADD} @LIBCRACK@ @LIBPAM@
su_LDADD = ${LDADD} @LIBPAM@

