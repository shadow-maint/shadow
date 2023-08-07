#! /bin/sh

autoreconf -v -f --install || exit 1

./configure \
	CFLAGS="-O2 -Wall" \
	--enable-lastlog \
	--enable-man \
	--enable-maintainer-mode \
	--enable-shared \
	--without-libpam \
	--with-selinux \
	"$@"
