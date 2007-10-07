Summary: Shadow password file utilities for Linux
Name: shadow-utils
Version: 970616
Release: 11
Source0: ftp://ftp.ists.pwr.wroc.pl/pub/linux/shadow/beta/shadow-970616.tar.gz
Source1: shadow-970616.login.defs
Source2: shadow-970616.useradd
Patch0: shadow-970616-rh.patch
Patch1: shadow-970616-utuser.patch
Patch2: shadow-970616-glibc.patch
Patch3: shadow-970616-fix.patch
Copyright: BSD
Group: Utilities/System
BuildRoot: /var/tmp/shadow-utils
Obsoletes: adduser

%changelog
* Tue Dec 30 1997 Cristian Gafton <gafton@redhat.com>
- updated the spec file
- updated the patch so that new accounts created on shadowed system won't
  confuse pam_pwdb anymore ('!!' default password instead on '!')
- fixed a bug that made useradd -G segfault
- the check for the ut_user is now patched into configure

* Thu Nov 13 1997 Erik Troan <ewt@redhat.com>
- added patch for XOPEN oddities in glibc headers
- check for ut_user before checking for ut_name -- this works around some
  confusion on glibc 2.1 due to the utmpx header not defining the ut_name
  compatibility stuff. I used a gross sed hack here because I couldn't make
  automake work properly on the sparc (this could be a glibc 2.0.99 problem
  though). The utuser patch works fine, but I don't apply it.
- sleep after running autoconf

* Thu Nov 06 1997 Cristian Gafton <gafton@redhat.com>
- added forgot lastlog command to the spec file

* Mon Oct 26 1997 Cristian Gafton <gafton@redhat.com>
- obsoletes adduser

* Thu Oct 23 1997 Cristian Gafton <gafton@redhat.com>
- modified groupadd; updated the patch

* Fri Sep 12 1997 Cristian Gafton <gafton@redhat.com>
- updated to 970616
- changed useradd to meet RH specs
- fixed some bugs

* Tue Jun 17 1997 Erik Troan <ewt@redhat.com>
- built against glibc

%description
This package includes the programs necessary to convert standard
UNIX password files to the shadow password format, as well as 
programs for command-line management of the user's accounts.
        - 'pwconv' converts everything to the shadow password format.
        - 'pwunconv' unconverts from shadow passwords, generating a file 
           in the current directory called npasswd that is a standard UNIX 
           password file.
        - 'pwck' checks the integrity of the password and shadow files.
        - 'lastlog' prints out the last login times of all users.
	- 'useradd', 'userdel' and 'usermod' for accounts management.
	- 'groupadd', 'groupdel' and 'groupmod' for group management.

A number of man pages are also included that relate to these utilities,
and shadow passwords in general.

%prep
# This is just a few of the core utilities from the shadow suite...
# packaged up for use w/PAM
%setup -n shadow-970616
%patch -p1 -b .rh
#%patch1 -p1 -b .utname
%patch2 -p1 -b .xopen
%patch3 -p1 -b .fix

%build
autoheader
autoconf
sleep 2
CFLAGS="$RPM_OPT_FLAGS" ./configure --prefix=/usr
make 

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr
make install prefix=/$RPM_BUILD_ROOT/usr
mkdir -p $RPM_BUILD_ROOT/etc/default
install -m 0600 -o root $RPM_SOURCE_DIR/shadow-970616.useradd $RPM_BUILD_ROOT/etc/default/useradd
install -m 0644 -o root $RPM_SOURCE_DIR/shadow-970616.login.defs $RPM_BUILD_ROOT/etc/login.defs
install -m 0500 -o root src/pwconv $RPM_BUILD_ROOT/usr/sbin
install -m 0500 -o root src/pwunconv $RPM_BUILD_ROOT/usr/sbin
ln -s pwconv $RPM_BUILD_ROOT/usr/sbin/pwconv5
install -m 0644 -o root man/pw*conv.8 $RPM_BUILD_ROOT/usr/man/man8
ln -s pwconv.8 $RPM_BUILD_ROOT/usr/man/man8/pwconv5.8
ln -s useradd $RPM_BUILD_ROOT/usr/sbin/adduser
ln -s useradd.8 $RPM_BUILD_ROOT/usr/man/man8/adduser.8

%files
%doc doc/ANNOUNCE doc/CHANGES doc/HOWTO
%doc doc/LICENSE doc/README doc/README.linux
%dir /etc/default
/usr/sbin/adduser
/usr/sbin/useradd
/usr/sbin/usermod
/usr/sbin/userdel
/usr/sbin/groupadd
/usr/sbin/groupdel
/usr/sbin/groupmod
/usr/sbin/grpck
/usr/sbin/pwck
/usr/bin/chage
/usr/bin/gpasswd
/usr/sbin/lastlog
# /usr/sbin/vipw
/usr/sbin/chpasswd
/usr/sbin/newusers
/usr/sbin/pw*conv*
/usr/man/man1/chage.1
/usr/man/man1/gpasswd.1
/usr/man/man3/shadow.3
/usr/man/man5/shadow.5
/usr/man/man8/adduser.8
/usr/man/man8/chpasswd.8
/usr/man/man8/group*.8
/usr/man/man8/user*.8
/usr/man/man8/pwck.8
/usr/man/man8/grpck.8
/usr/man/man8/newusers.8
# /usr/man/man8/shadowconfig.8
/usr/man/man8/pw*conv*.8
# /usr/man/man8/vipw.8
/usr/man/man8/lastlog.8
%config /etc/login.defs
%config /etc/default/useradd

%clean
rm -rf $RPM_BUILD_ROOT
