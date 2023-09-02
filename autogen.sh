#! /bin/sh

autoreconf -v -f --install "$(dirname "$0")" || exit 1

"$(dirname "$0")"/configure \
	CFLAGS="-O2 -Wall" \
	--enable-lastlog \
	--enable-man \
	--enable-maintainer-mode \
	--enable-shared \
	--without-libpam \
	--with-selinux \
	"$@"
