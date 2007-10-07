# shadow-utils.spec generated automatically from shadow-utils.spec.in
# $Id: shadow-utils.spec.in,v 1.2 1999/06/07 16:40:45 marekm Exp $
Summary: Shadow password file utilities for Linux
Name: shadow-utils
Version: 19990827
Release: 1
Copyright: Free
Group: Utilities/System
Source: ftp://ftp.ists.pwr.wroc.pl/pub/linux/shadow/shadow-19990827.tar.gz
BuildRoot: /var/tmp/shadow-utils
Packager: Timo Karjalainen <timok@iki.fi>
# Obsoletes: adduser

%description
This package includes the programs necessary to convert traditional
V7 UNIX password files to the SVR4 shadow password format and additional
tools to work with shadow passwords.
	- 'pwconv' converts everything to the shadow password format.
	- 'pwunconv' converts back to non-shadow passwords.
	- 'pwck' checks the integrity of the password and shadow files.
	- 'lastlog' prints out the last login times of all users.
	- 'useradd', 'userdel', 'usermod' to manage user accounts.
	- 'groupadd', 'groupdel', 'groupmod' to manage groups.

A number of man pages are also included that relate to these utilities,
and shadow passwords in general.

%changelog

* Sun Dec 14 1997 Marek Michalkiewicz <marekm@piast.t19.ds.pwr.wroc.pl>

- Lots of changes, see doc/CHANGES for more details

* Sun Jun 08 1997 Timo Karjalainen <timok@iki.fi>

- Initial release

%prep
# This is just a few of the core utilities from the shadow suite...
# packaged up for use w/PAM
%setup -n shadow-19990827

%build
# shared lib support is untested, so...
CFLAGS="$RPM_OPT_FLAGS" ./configure --disable-shared --prefix=/usr --exec-prefix=/usr
make

%install
if [ -d $RPM_BUILD_ROOT ] ; then
	rm -rf $RPM_BUILD_ROOT
fi
mkdir -p $RPM_BUILD_ROOT/usr
# neato trick, heh ? :-)
./configure --prefix=$RPM_BUILD_ROOT/usr
make install
mkdir -p $RPM_BUILD_ROOT/etc/default

# FIXME
#install -m 0600 -o root $RPM_SOURCE_DIR/shadow-970616.useradd $RPM_BUILD_ROOT/etc/default/useradd
#install -m 0644 -o root $RPM_SOURCE_DIR/shadow-970616.login.defs $RPM_BUILD_ROOT/etc/login.defs
#ln -s useradd $RPM_BUILD_ROOT/usr/sbin/adduser
#ln -s useradd.8 $RPM_BUILD_ROOT/usr/man/man8/adduser.8

#make prefix=$RPM_BUILD_ROOT/usr exec_prefix=$RPM_BUILD_ROOT/usr install
#touch $RPM_BUILD_ROOT/etc/{login.defs,default/useradd}
#chmod 640 $RPM_BUILD_ROOT/etc/{login.defs,default/useradd}
#chown root $RPM_BUILD_ROOT/etc/{login.defs,default/useradd}
#chgrp root $RPM_BUILD_ROOT/etc/{login.defs,default/useradd}

%clean
rm -rf $RPM_BUILD_ROOT

%files
%doc doc/ANNOUNCE doc/CHANGES doc/HOWTO
%doc doc/LICENSE doc/README doc/README.linux
%dir /etc/default
%config /etc/default/useradd
# %config /etc/limits
# %config /etc/login.access
%config /etc/login.defs
# %config /etc/limits
# %config /etc/porttime
# %config /etc/securetty
# %config /etc/shells
# %config /etc/suauth
# /bin/login
# /bin/su
/usr/bin/chage
# /usr/bin/chfn
# /usr/bin/chsh
# /usr/bin/expiry
# /usr/bin/faillog
/usr/bin/gpasswd
/usr/bin/lastlog
# /usr/bin/newgrp
# /usr/bin/passwd
# /usr/bin/sg
/usr/man/man1/chage.1
# /usr/man/man1/chfn.1
# /usr/man/man1/chsh.1
/usr/man/man1/gpasswd.1
# /usr/man/man1/login.1
# /usr/man/man1/passwd.1
# /usr/man/man1/sg.1
# /usr/man/man1/su.1
/usr/man/man3/shadow.3
# /usr/man/man5/faillog.5
# /usr/man/man5/limits.5
# /usr/man/man5/login.access.5
# /usr/man/man5/login.defs.5
# /usr/man/man5/passwd.5
# /usr/man/man5/porttime.5
/usr/man/man5/shadow.5
# /usr/man/man5/suauth.5
# /usr/man/man8/adduser.8
/usr/man/man8/chpasswd.8
# /usr/man/man8/faillog.8
/usr/man/man8/groupadd.8
/usr/man/man8/groupdel.8
/usr/man/man8/groupmod.8
/usr/man/man8/grpck.8
/usr/man/man8/lastlog.8
# /usr/man/man8/logoutd.8
/usr/man/man8/newusers.8
/usr/man/man8/pwck.8
/usr/man/man8/pwconv.8
# /usr/man/man8/shadowconfig.8
/usr/man/man8/useradd.8
/usr/man/man8/userdel.8
/usr/man/man8/usermod.8
# /usr/man/man8/vigr.8
# /usr/man/man8/vipw.8
# /usr/sbin/adduser
/usr/sbin/chpasswd
/usr/sbin/groupadd
/usr/sbin/groupdel
/usr/sbin/groupmod
/usr/sbin/grpck
/usr/sbin/grpconv
/usr/sbin/grpunconv
# /usr/sbin/logoutd
/usr/sbin/newusers
/usr/sbin/pwck
/usr/sbin/pwconv
/usr/sbin/pwunconv
# /usr/sbin/shadowconfig
/usr/sbin/useradd
/usr/sbin/userdel
/usr/sbin/usermod
# /usr/sbin/vigr
# /usr/sbin/vipw
